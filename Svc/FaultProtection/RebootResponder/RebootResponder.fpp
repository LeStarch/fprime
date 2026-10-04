module Svc {
module FaultProtection {
    @* Reboots the FSW in response to a fault
    @*
    @* A SyncResponder that performs a hard reboot when dispatched. The reboot is announced via event on dispatch and
    @* performed on the `run` tick a configurable number of ticks later, allowing the announcement to downlink without
    @* blocking the dispatching thread. The reboot itself is the (overridable) `doReboot` hook. The step never
    @* completes: the reboot ends the running software. Cancel requests are refused since a reboot is irrevocable
    @* once requested.
    passive component RebootResponder {
        import SyncResponder

        @ Rate group tick driving the delay between the reboot announcement and the reboot
        guarded input port run: Svc.Sched

        @ Hard reboot requested by a fault response step
        event RebootRequested(response: FaultConfig.Response, step: FaultConfig.Step) \
            severity warning high format "Hard reboot requested by {} step {}"

        @ Cancel requested for a reboot. Reboots cannot be canceled.
        event RebootCancelRefused severity warning low format \
            "Reboot cancel requested; a requested reboot is irrevocable"

        ###############################################################################
        # Standard AC Ports: Required for Channels, Events, Commands, and Parameters  #
        ###############################################################################
        @ Port for requesting the current time
        time get port timeCaller

        @ Port for sending textual representation of events
        text event port logTextOut

        @ Port for sending events to downlink
        event port logOut
    }
} # FaultProtection
} # Svc
