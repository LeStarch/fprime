@* FaultConfig:
@*
@* Module defining the configuration of the fault protection system. All defined constants and enumerations are
@* required for fault protection to function correctly. Enumeration values follow several conventions:
@*
@* REQUIRED(Component): This value is required to use the named component or an error will occur. 
@* REQUIRED LAST ELEMENT: This value is required to be the last item in the enumeration or an error will occur.
@* REQUIRED SEQUENCE RESPONDER: This value is required to use the sequence responder subtopology.
@* EXAMPLE: This value is an example.
@*
module FaultConfig {
    @ Number of steps defined for each response. Remember, extra steps must be filled with SKIP.
    constant FAULT_RESPONSE_STEP_COUNT = 3

    @* Number of FaultManager `run` ticks to wait after a fault report before starting the response
    @*
    @* This delay allows additional (possibly higher-precedence) fault reports to accumulate such that the response
    @* to the highest-precedence fault is selected. A value of 0 responds on the tick following the report.
    constant RESPONSE_COUNTDOWN_TICKS = 2

    @* Fault ID enumeration
    @*
    @* An enumeration of possible faults in the system. Projects shall add a unique enumerated fault value for each
    @* fault that any component can yield. 
    @*
    @* REQUIRED entries shall be maintained by the project when using the noted component.
    @*
    @* When a fault is announced, the supplied Fault maps to a given (enumerated) response that FaultManager will
    @* take. See: FaultConfig.Response.
    enum Fault : U8 {
        FATAL_OCCURRED @< REQUIRED (FatalToFault): a FATAL occurred and was translated into a fault
        FAULT_RESPONSE_FAILURE @< REQUIRED (FaultManager): fault for fault response failure reported by FaultManager
        NUM_FAULTS @< REQUIRED LAST ELEMENT: fault count
    }

    @* Fault response enumeration
    @*
    @* An enumeration of possible responses taken by FaultManager in response to a fault. Fault responses are named
    @* collections of fault response Steps that are dispatched to individual components. There are no REQUIRED
    @* responses.
    enum Response : U8 {
        REBOOT_RESPONSE   @< EXAMPLE: an example response used to trigger a software reboot
        SEQUENCE_RESPONSE @< EXAMPLE: an example response for sequence delegation
        SEQUENCE_THEN_REBOOT_RESPONSE @< EXAMPLE: an example multi-step response: run a sequence, then reboot
        NUM_RESPONSES @< REQUIRED LAST ELEMENT: response count
    }

    @* Fault response step
    @*
    @* An enumeration of the steps a response may dispatch. Each step is dispatched to a responder component through
    @* the port configured in the StepDefinitionTable. NUM_STEPS and SKIP are REQUIRED; SKIP fills unused step slots.
    enum Step : U8 {
        RUN_SEQUENCE @< EXAMPLE: dispatch to the SequenceResponder
        REBOOT    @< EXAMPLE: dispatch to the RebootResponder
        NUM_STEPS @< REQUIRED PENULTIMATE ELEMENT: step count
        SKIP      @< REQUIRED LAST ELEMENT: (FaultManager)
    }

    @* Port enumerations
    @*
    @* A enumeration of names describing the output ports from FaultManager. This is done so that multiple steps could
    @* be dispatched to a single responder component without requiring multiple ports outputs.
    @*
    @* This enumeration shall have one entry for each `stepDispatchOut` port of FaultManager.
    enum Port : U8 {
        SEQUENCE_RESPONDER_PORT @< EXAMPLE: port connected to the SequenceResponder component
        REBOOT_RESPONDER_PORT   @< EXAMPLE: port connected to the RebootResponder component
        NUM_PORTS               @< REQUIRED LAST ELEMENT: port count
    }

    @* Project defined context
    @*
    @* Context definition associated with a Step that will be forwarded to responding component. Framework responders
    @* shall ignore this context, but project defined responders can use this context.
    struct Context {
        example: U8 @< EXAMPLE: provide a U8 called 'example' for user context 
    }

    @* Fault response table
    @*
    @* This table defines the Fault properties: precedence of the fault, and the response taken to the fault.
    @* There shall be one entry for each enumerated value in the Fault enumeration.
    @*
    @* Entries in the table are the initial configuration.  Entries can be updated via a command to FaultManager.
    constant FaultResponseTable = [
        { fault = Fault.FATAL_OCCURRED,         precedence = 10, response = Response.REBOOT_RESPONSE, enabled = Fw.Enabled.ENABLED },
        { fault = Fault.FAULT_RESPONSE_FAILURE, precedence = 20, response = Response.REBOOT_RESPONSE, enabled = Fw.Enabled.ENABLED }
    ]
    
    @* Response definition table
    @*
    @* Definitions of responses as a set of individual steps dispatched to component. There shall be one entry for each
    @* response defined in the Response enumeration.
    @*
    @* Entries in the table are static configuration and cannot be updated at runtime.
    constant ResponseDefinitionTable = [
        { response = Response.SEQUENCE_RESPONSE,             steps = [Step.RUN_SEQUENCE, Step.SKIP, Step.SKIP] },
        { response = Response.REBOOT_RESPONSE,               steps = [Step.REBOOT, Step.SKIP, Step.SKIP] },
        { response = Response.SEQUENCE_THEN_REBOOT_RESPONSE, steps = [Step.RUN_SEQUENCE, Step.REBOOT, Step.SKIP] }
    ]

    @* Response step table
    @*
    @* Definitions of steps containing: failure mode, dispatchPort, timeout, and project-supplied context. There shall
    @* be one entry defined per step in the Step enumeration except for SKIP.
    @*
    @* - `failureMode` determines how FaultManager continues in the event of a failed response step status.
    @* - `dispatchPort` sets the step response output port enumeration
    @* - `timeoutTicks` bounds the wait for the step's completion in FaultManager `run` ticks (0 = no timeout)
    @* - `context` sets the project-supplied context to the call
    @*
    @* Entries in the table are the initial configuration. `failureMode` can be updated via command.
    constant StepDefinitionTable = [
        { step = Step.RUN_SEQUENCE, failureMode = FailureMode.FAULT, dispatchPort = Port.SEQUENCE_RESPONDER_PORT, timeoutTicks = 60, context = { example = 3 } },
        { step = Step.REBOOT,       failureMode = FailureMode.FAULT, dispatchPort = Port.REBOOT_RESPONDER_PORT,   timeoutTicks = 30, context = { example = 7 } }
    ]

}
