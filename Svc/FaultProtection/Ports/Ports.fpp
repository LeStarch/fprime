module Svc {
module FaultProtection {

    @* Port used to report a fault has occurred
    @*
    @* Faults report one of an enumerated set of fault IDs via the FaultReport port. Components needing to report
    @* faults shall instantiate the port with name `faultOut` and connect it to the FaultManager component.
    port FaultReport($id: FaultConfig.Fault @< ID of the reported fault
    )

    @* Port used to dispatch a fault response step to a responding component
    @*
    @* Each step of a fault response is dispatched using this port. FaultManager shall call the FaultResponseDispatch
    @* port attached to the responding component.
    port FaultResponseDispatch(
        response: FaultConfig.Response, @< Active fault response
        step: FaultConfig.Step, @< Step of the active fault response
        context: FaultConfig.Context @< Context of the step of the active fault response
    )

    @* Port used to complete the fault response
    @*
    @* When a responding component completes a fault response step it shall notify FaultManager via this port. It
    @* shall supply the response and step in the return call.
    port FaultResponseComplete(
        status: Fw.Success, @< Status of the fault response
        response: FaultConfig.Response, @< Active fault response
        step: FaultConfig.Step, @< Step of the active fault response
    )
} # FaultProtection
} # Svc
