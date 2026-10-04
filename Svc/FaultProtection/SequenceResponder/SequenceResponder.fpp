module Svc {
module FaultProtection {
    @* Responds to faults by running a command sequence
    @*
    @* A SyncResponder that delegates each dispatched step to a command sequencer (e.g. Svc.CmdSequencer) through the
    @* `seqRunOut` port. The sequence file run for a step is `<directory>/<step name>.seq` where the directory is set at
    @* initialization (`<directory>/<step value>.seq` when FW_SERIALIZABLE_TO_STRING is disabled). Sequence completion
    @* arrives on `seqDoneIn` and is forwarded as the step's completion status. Only one step may be in progress at a
    @* time: a dispatch while busy completes immediately with failure.
    passive component SequenceResponder {
        import SyncResponder

        @ Request a sequence run from the sequencer
        output port seqRunOut: Svc.CmdSeqIn

        @ Request cancellation of the running sequence
        output port seqCancelOut: Svc.CmdSeqCancel

        @ Sequence completion status from the sequencer
        guarded input port seqDoneIn: Fw.CmdResponse

        @ Sequence run requested for a fault response step
        event SequenceStarted(step: FaultConfig.Step, response: FaultConfig.Response, \
                              file: string size FileNameStringSize) \
            severity activity high format "{} of {} started sequence {}"

        @ Sequence for a fault response step completed successfully
        event SequenceCompleted(step: FaultConfig.Step, response: FaultConfig.Response) \
            severity activity high format "{} of {} sequence completed"

        @ Sequence for a fault response step failed
        event SequenceFailed(step: FaultConfig.Step, response: FaultConfig.Response, status: Fw.CmdResponse) \
            severity warning high format "{} of {} sequence failed with status {}"

        @ Sequence for a fault response step canceled
        event SequenceCanceled(step: FaultConfig.Step, response: FaultConfig.Response) \
            severity activity high format "{} of {} sequence canceled"

        @ A step was dispatched while another step's sequence was still running
        event ResponderBusy(step: FaultConfig.Step, response: FaultConfig.Response) \
            severity warning high format "{} of {} rejected: a fault response sequence is already running"

        @ Sequencer reported completion when no fault response sequence was in progress
        event UnexpectedSequenceDone(status: Fw.CmdResponse) \
            severity warning low format "Sequence completion {} received with no fault response sequence running"

        @ The sequence run port is not connected: configuration error
        event SequencerUnconnected severity warning high format \
            "Sequence run port is not connected; step treated as failed"

        @ The sequence file name does not fit in a file name string: configuration error
        event SequenceFileNameTooLong(step: FaultConfig.Step) severity warning high format \
            "Sequence file name for {} exceeds the file name size; step treated as failed"

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
