@* FaultConfig (Ref deployment override):
@*
@* Fault protection configuration for the Ref deployment. Extends the framework defaults with the COUNTER_HIGH fault
@* reported by Ref.MonitoredCounter and a two-step sequence response correcting it. See the framework FaultConfig.fpp
@* for the conventions (REQUIRED, REQUIRED LAST ELEMENT, ...) that apply to every entry.
module FaultConfig {
    @ Number of steps defined for each response. Remember, extra steps must be filled with SKIP.
    constant FAULT_RESPONSE_STEP_COUNT = 3

    @ Number of FaultManager `run` ticks to wait after a fault report before starting the response
    constant RESPONSE_COUNTDOWN_TICKS = 2

    @ Fault ID enumeration
    enum Fault : U8 {
        FATAL_OCCURRED         @< REQUIRED (FatalToFault): a FATAL occurred and was translated into a fault
        FAULT_RESPONSE_FAILURE @< REQUIRED (FaultManager): fault for fault response failure reported by FaultManager
        COUNTER_HIGH           @< Ref.MonitoredCounter: the count has exceeded its threshold
        NUM_FAULTS             @< REQUIRED LAST ELEMENT: fault count
    }

    @ Fault response enumeration
    enum Response : U8 {
        REBOOT_RESPONSE        @< Hard reboot of the software
        RESET_COUNTER_RESPONSE @< Reset the monitored counter by sequence, then acknowledge by sequence
        NUM_RESPONSES          @< REQUIRED LAST ELEMENT: response count
    }

    @ Fault response step enumeration. Sequence steps run `<directory>/<step name>.seq`.
    enum Step : U8 {
        RESET_COUNT_SEQUENCE @< Sequence commanding Ref.monitoredCounter.RESET_COUNT
        ACKNOWLEDGE_SEQUENCE @< Sequence announcing completion of the response
        REBOOT               @< Dispatch to the RebootResponder
        NUM_STEPS            @< REQUIRED PENULTIMATE ELEMENT: step count
        SKIP                 @< REQUIRED LAST ELEMENT: (FaultManager)
    }

    @ Port enumerations: one entry for each dispatch output port from FaultManager
    enum Port : U8 {
        SEQUENCE_RESPONDER_PORT @< Port connected to the SequenceResponder component
        REBOOT_RESPONDER_PORT   @< Port connected to the RebootResponder component
        NUM_PORTS               @< REQUIRED LAST ELEMENT: port count
    }

    @ Project defined context forwarded to responders
    struct Context {
        example: U8 @< Unused by the Ref deployment
    }

    @ Fault response table: one entry per fault
    constant FaultResponseTable = [
        { fault = Fault.FATAL_OCCURRED,         precedence = 10, response = Response.REBOOT_RESPONSE, enabled = Fw.Enabled.ENABLED },
        { fault = Fault.FAULT_RESPONSE_FAILURE, precedence = 20, response = Response.REBOOT_RESPONSE, enabled = Fw.Enabled.ENABLED },
        { fault = Fault.COUNTER_HIGH,           precedence = 5,  response = Response.RESET_COUNTER_RESPONSE, enabled = Fw.Enabled.ENABLED }
    ]

    @ Response definition table: one entry per response
    constant ResponseDefinitionTable = [
        { response = Response.REBOOT_RESPONSE,        steps = [Step.REBOOT, Step.SKIP, Step.SKIP] },
        { response = Response.RESET_COUNTER_RESPONSE, steps = [Step.RESET_COUNT_SEQUENCE, Step.ACKNOWLEDGE_SEQUENCE, Step.SKIP] }
    ]

    @ Step definition table: one entry per step except SKIP. Timeouts are in 1 Hz FaultManager ticks.
    constant StepDefinitionTable = [
        { step = Step.RESET_COUNT_SEQUENCE, failureMode = FailureMode.FAULT,  dispatchPort = Port.SEQUENCE_RESPONDER_PORT, timeoutTicks = 30, context = { example = 0 } },
        { step = Step.ACKNOWLEDGE_SEQUENCE, failureMode = FailureMode.IGNORE, dispatchPort = Port.SEQUENCE_RESPONDER_PORT, timeoutTicks = 30, context = { example = 0 } },
        { step = Step.REBOOT,               failureMode = FailureMode.FAULT,  dispatchPort = Port.REBOOT_RESPONDER_PORT,   timeoutTicks = 30, context = { example = 0 } }
    ]
}
