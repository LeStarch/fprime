module Svc {
module FaultProtection {
    @* Translates FATAL events into FATAL_OCCURRED fault reports
    @*
    @* A drop-in replacement for Svc.FatalHandler: when a FATAL is announced, the fault FATAL_OCCURRED is reported to
    @* the FaultManager so the configured response (typically a reboot) runs. The asserting thread is then parked for
    @* a configurable interval: a FATAL means execution must not continue on that thread. If the software is still
    @* running when the interval expires, the fallback (abort) is invoked.
    passive component FatalToFault {
        import Svc.FaultProtection.Reporter

        @ Incoming FATAL event announcement
        sync input port FatalReceive: Svc.FatalEvent
    }
} # FaultProtection
} # Svc
