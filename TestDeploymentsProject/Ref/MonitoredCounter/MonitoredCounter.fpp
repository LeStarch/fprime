module Ref {

    @ Color of a fault monitor
    enum MonitorColor : U8 {
        BLACK  @< Monitoring precondition not met; monitor inactive
        GREEN  @< Monitored test passing
        YELLOW @< Local error threshold exceeded; local response taken
        RED    @< System error threshold exceeded; fault reported
    }

    @* Counts rate group cycles and monitors the count for excess
    @*
    @* A demonstration of the fault protection monitor pattern. Each `run` cycle increments a counter and cycles the
    @* Svc.FaultProtection.MonitorMachine. The monitor's test is "count exceeds COUNT_THRESHOLD". Consecutive
    @* cycles failing the test raise the error count; crossing LOCAL_ERROR_THRESHOLD turns the monitor YELLOW (a
    @* warning), crossing SYSTEM_ERROR_THRESHOLD turns it RED and reports the fault COUNTER_HIGH. The expected
    @* fault response runs a sequence that commands RESET_COUNT, after which the monitor returns to GREEN.
    active component MonitoredCounter {

        @ Monitor state machine evaluating the count each cycle
        state machine instance monitorMachine: Svc.FaultProtection.MonitorMachine

        import Svc.FaultProtection.Reporter

        @ Rate group cycle: increments the count and cycles the monitor
        async input port run: Svc.Sched

        @ Reset the count to zero (the fault corrective action)
        async command RESET_COUNT

        @ Enable or disable monitoring (the monitor's precondition)
        async command SET_MONITORING(enabled: Fw.Enabled)

        @ Count above which the monitor's test fails
        param COUNT_THRESHOLD: U32 default 10

        @ Error count at which the monitor turns YELLOW
        param LOCAL_ERROR_THRESHOLD: U32 default 2

        @ Error count at which the monitor turns RED and reports COUNTER_HIGH
        param SYSTEM_ERROR_THRESHOLD: U32 default 4

        @ Count was reset by command
        event CountReset(previous: U32) severity activity high format "Count reset from {}"

        @ Monitoring enabled state changed by command
        event MonitoringSet(enabled: Fw.Enabled) severity activity high format "Monitoring set {}"

        @ Monitor color changed
        event MonitorColorChanged(previous: MonitorColor, current: MonitorColor, count: U32, errors: U32) \
            severity activity low format "Monitor {} -> {} at count {} with {} errors"

        @ Local response: monitor turned YELLOW
        event CountHighWarning(count: U32, errors: U32) \
            severity warning low format "Count {} high for {} cycles: local response taken"

        @ System response: monitor turned RED and reported COUNTER_HIGH
        event CountHighFault(count: U32, errors: U32) \
            severity warning high format "Count {} high for {} cycles: fault COUNTER_HIGH reported"

        @ Current count
        telemetry Count: U32

        @ Current monitor error count
        telemetry ErrorCount: U32

        @ Current monitor color
        telemetry Monitor: MonitorColor

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
}
