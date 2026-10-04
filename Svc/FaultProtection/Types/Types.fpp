#* Types.fpp:
#*
#* Module containing the data types associated with FaultProtection. This grouping contains the data types that express
#* the shape of constants defined in FaultConfig and used in FaultProtection components.
module Svc {
module FaultProtection {

    @* Failure handling mode
    @*
    @* Defines the different options for handling failure statuses returned by a Step dispatch. Each Step shall be
    @* configured selecting how failures will be handled.
    enum FailureMode {
        IGNORE  @< IGNORE will ignore the error and continue with subsequent steps
        DEFER   @< DEFER will trigger FAULT_RESPONSE_FAILURE after all steps have run
        FAULT   @< FAULT will trigger FAULT_RESPONSE_FAILURE immediately
    }

    @* Fault response table entry
    @*
    @* The Fault response table maps an incoming FaultId to the appropriate Response. It configures precedence allowing
    @* a higher-priority fault to cancel a lower priority fault and start the higher-priority Response.
    @*
    @* There shall be one fault response table entry for each faultId specified in then FaultConfig.FaultId enumeration.
    @* a given Response may be mapped to multiple FaultIds.
    struct FaultResponseEntry {
        fault: FaultConfig.Fault @< Fault ID to map
        precedence: U8 @< Precedence of fault where 255 is the highest, and 0 is the lowest
        response: FaultConfig.Response @< Response to take
        enabled: Fw.Enabled @< Is the response to this fault ENABLED/DISABLED
    } default { enabled = Fw.Enabled.ENABLED }

    @* Definition of a fix-length array of steps
    @*
    @* Shape of the steps element of the ResponseDefinitionEntry. In order to parse the defaults correctly, FPP needed
    @* this shape explicitly defined.
    array Steps = [FaultConfig.FAULT_RESPONSE_STEP_COUNT] FaultConfig.Step

    @* Definition of a response to a fault
    @*
    @* A fault Response consists of a series of Steps. The response definition table entry maps a Response enumeration
    @* to the array of steps representing that response. The length of the array of steps is configurable via the
    @* FaultConfig.FAULT_RESPONSE_STEP_COUNT constant.
    @*
    @* There shall be one response definition table entry for each Response in the FaultConfig.Response enumeration.
    @* Unused elements in the steps array shall be set to FaultConfig.STEP.
    struct ResponseDefinitionEntry {
        response: FaultConfig.Response @< Response to map
        steps: Steps @< Set of steps to take
    }

    @* Step definition of a response
    @*
    @* Definition of a Step. Maps the configurable Step enumeration to a configured failure handling mode dispatch port
    @* and context. Failure mode is used to indicate how failure responses from the responding component are handled.
    @* See: FailureMode. Dispatch port is the port enumeration to invoke and context is project-supplied context.
    @*
    @* There shall be one step definition table entry for each Step defined in FaultConfig.Step.
    struct StepDefinitionEntry {
        step: FaultConfig.Step @< Step to map
        failureMode: FaultConfig.FailureMode @< Action taken when this step completes with failure status
        dispatchPort: FaultConfig.Port @< Port dispatched to
        context: FaultConfig.Context @< Context to supply to dispatch
    }
}
}