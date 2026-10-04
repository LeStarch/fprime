module Svc {
module FaultProtection {
    # ----------------------------------------------------------------------
    # Passive Components
    # ----------------------------------------------------------------------
    instance sequenceResponder: SequenceResponder base id FaultProtection.BASE_ID + 0x00002000
    instance rebootResponder: RebootResponder base id FaultProtection.BASE_ID + 0x00003000

    # ----------------------------------------------------------------------
    # Active Components
    # ----------------------------------------------------------------------
    instance faultManager: FaultManager base id FaultProtection.BASE_ID + 0x00004000 \
        queue size FaultProtection.QueueSizes.faultManager \
        stack size FaultProtection.StackSizes.faultManager \
        priority FaultProtection.Priorities.faultManager

    @* Fault protection subtopology
    @*
    @* Wires the FaultManager to the framework responders. Fault reporters connect to the `reportIn` port. FATAL
    @* translation is achieved by configuring CdhCore's `fatalHandler` instance as `Svc.FaultProtection.FatalToFault`
    @* and connecting its `faultOut` to `reportIn`.
    topology Subtopology {
        instance faultManager
        instance sequenceResponder
        instance rebootResponder

        connections Responders {
            faultManager.stepDispatchOut[FaultConfig.Port.SEQUENCE_RESPONDER_PORT] -> sequenceResponder.faultResponseDispatch
            faultManager.stepDispatchOut[FaultConfig.Port.REBOOT_RESPONDER_PORT] -> rebootResponder.faultResponseDispatch
            faultManager.stepCancelOut[FaultConfig.Port.SEQUENCE_RESPONDER_PORT] -> sequenceResponder.faultResponseCancel
            faultManager.stepCancelOut[FaultConfig.Port.REBOOT_RESPONDER_PORT] -> rebootResponder.faultResponseCancel
            sequenceResponder.faultResponseComplete -> faultManager.stepCompletionIn
            rebootResponder.faultResponseComplete -> faultManager.stepCompletionIn
        }

        # ----------------------------------------------------------------------
        # Topology ports
        # ----------------------------------------------------------------------

        @ Input port for fault reports from any fault reporter
        port reportIn = faultManager.reportIn

        @ Input port for the rate group tick driving the FaultManager
        port faultManagerRun = faultManager.run

        @ Input port for the rate group tick driving the RebootResponder's reboot delay
        port rebootResponderRun = rebootResponder.run

        @ Output port requesting a sequence run from the fault response sequencer
        port seqRunOut = sequenceResponder.seqRunOut

        @ Output port canceling the fault response sequencer
        port seqCancelOut = sequenceResponder.seqCancelOut

        @ Input port for fault response sequencer completion
        port seqDoneIn = sequenceResponder.seqDoneIn
    }
} # FaultProtection
} # Svc
