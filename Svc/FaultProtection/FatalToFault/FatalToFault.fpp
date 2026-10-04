module Svc {
module FaultProtection {
    @* Translates FATAL events into FATAL_OCCURRED fault reports
    @*
    @* A drop-in replacement for Svc.FatalHandler: when a FATAL is announced, the fault FATAL_OCCURRED is reported to
    @* the FaultManager so the configured response (typically a reboot) runs. As a safety net, the `run` port counts
    @* down a configurable number of ticks after the first FATAL; if the software is still running when the countdown
    @* expires, the fallback (abort) is invoked.
    passive component FatalToFault {
        import Svc.FaultProtection.Reporter

        @ Incoming FATAL event announcement
        sync input port FatalReceive: Svc.FatalEvent

        @ Rate group tick driving the fallback countdown. If unconnected, no fallback occurs.
        sync input port run: Svc.Sched
    }
} # FaultProtection
} # Svc
