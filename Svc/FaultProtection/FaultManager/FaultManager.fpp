module Svc {
module FaultProtection {

    @ Fault response table whose default value is the configuration fault response table
    array FaultResponseTable = [FaultConfig.Fault.NUM_FAULTS] FaultResponseEntry default FaultConfig.FaultResponseTable

    @ Response definition table whose default value is the configuration response definition table
    array ResponseDefinitionTable = [FaultConfig.Response.NUM_RESPONSES] ResponseDefinitionEntry default FaultConfig.ResponseDefinitionTable

    @ Step definition table whose default value is the configuration step definition table
    array StepDefinitionTable = [FaultConfig.Step.NUM_STEPS] StepDefinitionEntry default FaultConfig.StepDefinitionTable

    @ Enabled/disabled for each and every response. Responses are enabled by default.
    array ResponsesEnabled = [FaultConfig.Response.NUM_RESPONSES] Fw.Enabled default Fw.Enabled.ENABLED

    @ Failure mode of each step
    array StepFailureModes = [FaultConfig.Step.NUM_STEPS] FaultConfig.FailureMode

    @* State machine for handling fault response execution
    @*
    @* The machine idles until a tick finds a latched fault report. It then counts down a configurable number of ticks
    @* (allowing further reports to accumulate) before selecting the response for the highest-precedence latched fault
    @* and dispatching that response's steps one at a time. Each step completes with success, failure, or deferred
    @* failure, or is preempted by a higher-precedence fault report. The choices CHECK_COUNTDOWN and CHECK_RESPONSE are
    @* nested within their parent states so that evaluating them does not re-run the parent's entry/exit actions.
    state machine FaultManagerStateMachine {
        @ Signal indicating a tick of the rate group
        signal Tick

        @ Signal indicating a step has been completed
        signal StepSuccessful

        @ Signal indicating a step has failed
        signal StepFailed

        @ Signal indicating a step as completed, with a failure deferred until later
        signal StepDeferredFailure

        @ Signal indicating a higher-precedence fault has been reported and the active response must yield
        signal Preempt

        @ Check if there is a fault report
        guard hasReport

        @ Check if countdown has expired
        guard countdownExpired

        @ Check if response is done executing each step
        guard responseDone

        @ Action to start countdown
        action startCountdown

        @ Action to decrement countdown
        action decrementCountdown

        @ Action to select response to execute
        action selectResponse

        @ Action to complete (clean up after) the active response
        action completeResponse

        @ Action to dispatch a response step
        action dispatchStep

        @ Action to count a tick against the active step's timeout, failing the step when it expires
        action tickStep

        @ When a report is detected, enter COUNTDOWN to allow for additional reports to be processed before executing
        @ response otherwise return to the IDLE state to await the next tick and check again.
        choice CHECK_REPORT {
            if hasReport enter COUNTDOWN else enter IDLE
        }

        @ Enter IDLE state on initialization
        initial enter IDLE

        @ IDLE state: wait for a tick that finds a latched fault report; other signals are ignored
        state IDLE {
            on Tick enter CHECK_REPORT
        }

        @ COUNTDOWN state: wait for the countdown to expire allowing reports to accumulate
        state COUNTDOWN {
            initial enter COUNTDOWN_ACTIVE
            @ Reset the countdown on enter
            entry do { startCountdown }

            @ Check if the countdown allowing other reports to come in has expired. If so, dispatch the next step in a
            @ new response. If not, remain in COUNTDOWN_ACTIVE awaiting the next tick.
            choice CHECK_COUNTDOWN {
                if countdownExpired enter RESPONSE else enter COUNTDOWN_ACTIVE
            }

            @ COUNTDOWN_ACTIVE state: wait for countdown to expire without resetting countdown
            state COUNTDOWN_ACTIVE {
                @ Decrement then check the countdown
                on Tick do { decrementCountdown } enter CHECK_COUNTDOWN
            }
        }

        @ RESPONSE state: process a series of response steps
        state RESPONSE {
            initial enter DISPATCH_STEP
            @ When entering the RESPONSE state, select the response to execute
            entry do { selectResponse }

            @ When leaving the RESPONSE state, complete the response
            exit do { completeResponse }

            @ Check if the response is done executing all steps. If so return to the CHECK_REPORT check for new fault
            @ reports otherwise dispatch the next step in the response.
            choice CHECK_RESPONSE {
                if responseDone enter CHECK_REPORT else enter DISPATCH_STEP
            }

            @ DISPATCH_STEP state: dispatch a response step
            state DISPATCH_STEP {
                entry do { dispatchStep }

                @ Each tick counts against the active step's timeout
                on Tick do { tickStep }

                @ When a step succeeds, check if the response is done
                on StepSuccessful enter CHECK_RESPONSE

                @ When a step fails, set response failure and return to CHECK_REPORT for new reports
                on StepFailed enter CHECK_REPORT

                @ When a step defers failure, set the response failure, but continue with checking for more steps
                on StepDeferredFailure enter CHECK_RESPONSE

                @ When preempted, cancel the running step and return to CHECK_REPORT to select the new response
                on Preempt enter CHECK_REPORT
            }
        }
    }

    @* Translates incoming Fault reports into outgoing fault response Steps
    @*
    @* Fault reports arrive synchronously on `reportIn` and are latched. The component's `run` port drives the
    @* FaultManagerStateMachine, which selects the response to the highest-precedence latched fault and dispatches the
    @* response's steps sequentially to the configured responder ports. Step completions arrive on `stepCompletionIn`.
    active component FaultManager {
        @ Instantiate the FaultManagerStateMachine as the primary implementation mechanism for the FaultManager
        state machine instance faultManagerStateMachine: FaultManagerStateMachine

        @* Set a fault enabled state
        @*
        @* Enable/disable response to the supplied Fault ID. This will update the internal parameter and may be
        @* persisted by FAULT_RESPONSE_TABLE_PRM_SAVE. Command is dropped on overflow to prevent triggering fault
        @* response.
        async command SET_FAULT_ENABLED(fault: FaultConfig.Fault, enabled: Fw.Enabled) drop

        @* Set a response enabled state
        @*
        @* Enable/disable response. This will update the internal parameter and may be persisted by
        @* RESPONSE_TABLE_PRM_SAVE. Command is dropped on overflow to prevent triggering fault response.
        async command SET_RESPONSE_ENABLED(response: FaultConfig.Response, enabled: Fw.Enabled) drop

        @* Set a response step failure mode
        @*
        @* Set the FAILURE_MODE of response step. This will update the internal parameter and may be persisted by
        @* STEP_TABLE_PRM_SAVE. Command is dropped on overflow to prevent triggering fault response.
        async command UPDATE_STEP_FAILURE_MODE(step: FaultConfig.Step, failureMode: FaultConfig.FailureMode) drop

        @ Rate group tick driving the fault response state machine. Dropped on overflow: the next tick will arrive.
        async input port run: Svc.Sched drop

        @ Incoming fault report. Synchronous such that a report is latched regardless of queue state.
        sync input port reportIn: FaultReport

        @ Outgoing response step dispatch
        output port stepDispatchOut: [FaultConfig.Port.NUM_PORTS] FaultResponseDispatch

        @ Outgoing response step cancel
        output port stepCancelOut: [FaultConfig.Port.NUM_PORTS] Fw.Signal

        @* Incoming response step completion. Dropped on overflow to avoid asserting (FATAL) within fault handling: a
        @* dropped completion is recovered by the step's timeout.
        async input port stepCompletionIn: FaultResponseComplete drop

        @ Internal port for processing a fault report on the component's thread: events, telemetry, the disabled-fault
        @ latch clear, and the preemption check. The latch itself is set synchronously in reportIn, so a dropped
        @ message delays preemption/diagnostics to the next tick, where hasReport finds the latched report; the report
        @ is never lost.
        internal port handleReport(fault: FaultConfig.Fault, latched: bool) drop

        @ Event indicating a fault was reported and latched. Latching bounds this event to one per fault per response.
        event FaultReported(fault: FaultConfig.Fault) severity activity high format "Fault {} reported"

        @* Event indicating a fault was reported while already latched and awaiting (or undergoing) response.
        @* Throttled per `run` tick: the throttle is cleared on each tick.
        event FaultIgnored(fault: FaultConfig.Fault) severity warning low format \
            "Fault {} reported and ignored: already latched and awaiting response" throttle 5

        @* Event indicating a fault was reported and ignored due to being disabled.
        @* Throttled per `run` tick: the throttle is cleared on each tick.
        event FaultDisabled(fault: FaultConfig.Fault) severity warning low format \
            "Fault {} reported and ignored: fault is disabled" throttle 5

        @* Event indicating a fault was reported with an ID outside the configured fault table.
        @* Throttled per `run` tick: the throttle is cleared on each tick.
        event FaultInvalid($id: U8) severity warning high format \
            "Fault ID {} reported and ignored: not a configured fault" throttle 5

        @ Fault response started
        event ResponseStarted(response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity high format "{} started, triggered by {}"

        @ Fault response completed successfully
        event ResponseCompleted(response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity high format "{} completed, triggered by {}"

        @ Fault response completed with failure. FAULT_RESPONSE_FAILURE will be reported.
        event ResponseFailed(response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity warning high format "{} failed, triggered by {}"

        @ The response to FAULT_RESPONSE_FAILURE itself failed: fault protection has no further escalation
        event EscalationExhausted(response: FaultConfig.Response) \
            severity warning high format "{} to FAULT_RESPONSE_FAILURE failed; escalation exhausted"

        @ Fault response preempted by a higher-precedence fault. The triggering fault remains latched.
        event ResponsePreempted(response: FaultConfig.Response, fault: FaultConfig.Fault, by: FaultConfig.Fault) \
            severity warning low format "{} triggered by {} preempted by higher-precedence fault {}"

        @ Fault response step started
        event StepStarted(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity low format "{} started as part of {} triggered by {}"

        @ Fault response step completed
        event StepCompleted(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity low format "{} completed as part of {} triggered by {}"

        @ Fault response step completed with failure status
        event StepFailed(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault, \
                         failureMode: FaultConfig.FailureMode) \
            severity warning high format "{} failed as part of {} triggered by {}; failure mode {}"

        @ Fault response step did not complete within its configured timeout: canceled and treated as failed
        event StepTimedOut(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity warning high format "{} of {} triggered by {} timed out; step treated as failed"

        @ Fault response step dispatch port is not connected: configuration error, step treated as failed
        event StepPortUnconnected(step: FaultConfig.Step, $port: FaultConfig.Port) \
            severity warning high format "{} dispatch port {} is not connected; step treated as failed"

        @ Unexpected fault response step completed
        event UnexpectedStepCompleted(step: FaultConfig.Step, response: FaultConfig.Response) \
            severity warning high format "Unexpected completion of {} as part of {}"

        @ Fault response step cancel requested
        event StepCancel(step: FaultConfig.Step) \
            severity activity high format "{} cancel requested"

        @ Fault response was disabled and thus its step was skipped
        event StepSkipped(step: FaultConfig.Step, response: FaultConfig.Response, fault: FaultConfig.Fault) \
            severity activity low format "{} skipped: {} (triggered by {}) is disabled"

        @ Fault enabled state changed by command
        event FaultEnabledSet(fault: FaultConfig.Fault, enabled: Fw.Enabled) \
            severity activity high format "Fault {} set {}"

        @ Response enabled state changed by command
        event ResponseEnabledSet(response: FaultConfig.Response, enabled: Fw.Enabled) \
            severity activity high format "{} set {}"

        @ Step failure mode changed by command
        event StepFailureModeSet(step: FaultConfig.Step, failureMode: FaultConfig.FailureMode) \
            severity activity high format "{} failure mode set to {}"

        @ Command fault argument was out of range of the configured faults
        event InvalidFaultArgument(value: U8) \
            severity warning low format "Fault argument {} is not a configured fault"

        @ Command response argument was out of range of the configured responses
        event InvalidResponseArgument(value: U8) \
            severity warning low format "Response argument {} is not a configured response"

        @ Command step argument was out of range of the configured steps
        event InvalidStepArgument(value: U8) \
            severity warning low format "Step argument {} is not a configured step"

        @ Count of faults reported
        telemetry FaultsReported: FwSizeType

        @ Count of faults ignored (duplicate, disabled, or invalid)
        telemetry FaultsIgnored: FwSizeType

        @ Count of responses completed successfully
        telemetry ResponsesCompleted: FwSizeType

        @ Count of responses that failed
        telemetry ResponsesFailed: FwSizeType

        @ Fault response setting table
        external param FAULT_RESPONSE_TABLE: FaultResponseTable default FaultConfig.FaultResponseTable

        @ Response enabled table
        external param RESPONSE_TABLE: ResponsesEnabled

        @ Step failure mode table
        external param STEP_TABLE: StepFailureModes

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending command registrations
        command reg port cmdRegOut

        @ Port for receiving commands
        command recv port cmdIn

        @ Port for sending command responses
        command resp port cmdResponseOut

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut

        @ Port for sending telemetry channels to downlink
        telemetry port tlmOut

        @ Port for retrieving parameters
        param get port prmGet

        @ Port for setting parameters
        param set port prmSet
    }

} # FaultProtection
} # Svc
