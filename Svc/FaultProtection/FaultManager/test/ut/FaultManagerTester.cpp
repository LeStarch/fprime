// ======================================================================
// \title  FaultManagerTester.cpp
// \author mstarch
// \brief  cpp file for FaultManager component test harness implementation class
// ======================================================================

#include "FaultManagerTester.hpp"

namespace Svc {

namespace FaultProtection {

namespace {
const FaultConfig::Fault FATAL = FaultConfig::Fault::FATAL_OCCURRED;
const FaultConfig::Fault FAILURE = FaultConfig::Fault::FAULT_RESPONSE_FAILURE;
const FaultConfig::Response REBOOT_RESPONSE = FaultConfig::Response::REBOOT_RESPONSE;
const FaultConfig::Response SEQUENCE_RESPONSE = FaultConfig::Response::SEQUENCE_RESPONSE;
const FaultConfig::Response SEQUENCE_THEN_REBOOT = FaultConfig::Response::SEQUENCE_THEN_REBOOT_RESPONSE;
const FaultConfig::Step REBOOT = FaultConfig::Step::REBOOT;
const FaultConfig::Step RUN_SEQUENCE = FaultConfig::Step::RUN_SEQUENCE;
const FaultConfig::Port REBOOT_PORT = FaultConfig::Port::REBOOT_RESPONDER_PORT;
const FaultConfig::Port SEQUENCE_PORT = FaultConfig::Port::SEQUENCE_RESPONDER_PORT;
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

FaultManagerTester ::FaultManagerTester()
    : FaultManagerGTestBase("FaultManagerTester", FaultManagerTester::MAX_HISTORY_SIZE),
      component("FaultManager"),
      m_last_dispatch_port(-1),
      m_last_cancel_port(-1) {
    this->initComponents();
    this->connectPorts();
    // The component is its own external parameter delegate: load the defaults through the tester's parameter store
    this->component.loadParameters();
    this->clearHistory();
}

FaultManagerTester ::~FaultManagerTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void FaultManagerTester ::testIdle() {
    this->tick(5);
    ASSERT_EVENTS_SIZE(0);
    this->assertNotDispatched();
}

void FaultManagerTester ::testNominalResponse() {
    this->report(FATAL);
    ASSERT_EVENTS_FaultReported_SIZE(1);
    ASSERT_EVENTS_FaultReported(0, FATAL);
    ASSERT_TLM_FaultsReported(0, 1);
    this->clearHistory();

    // Report is latched: the countdown holds the response for RESPONSE_COUNTDOWN_TICKS further ticks
    this->tick(TICKS_TO_RESPONSE - 1);
    this->assertNotDispatched();
    ASSERT_EVENTS_ResponseStarted_SIZE(0);

    this->tick();
    ASSERT_EVENTS_ResponseStarted_SIZE(1);
    ASSERT_EVENTS_ResponseStarted(0, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_StepStarted_SIZE(1);
    ASSERT_EVENTS_StepStarted(0, REBOOT, REBOOT_RESPONSE, FATAL);
    this->assertDispatched(REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->clearHistory();

    // Further ticks while awaiting completion do nothing
    this->tick(3);
    this->assertNotDispatched();
    ASSERT_EVENTS_SIZE(0);

    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_StepCompleted_SIZE(1);
    ASSERT_EVENTS_StepCompleted(0, REBOOT, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
    ASSERT_EVENTS_ResponseCompleted(0, REBOOT_RESPONSE, FATAL);
    ASSERT_TLM_ResponsesCompleted(0, 1);
    this->clearHistory();

    // Latch cleared: a new report of the same fault is accepted and responded to again
    this->tick(3);
    this->assertNotDispatched();
    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
}

void FaultManagerTester ::testDuplicateReportIgnored() {
    this->report(FATAL);
    this->clearHistory();
    this->report(FATAL);
    ASSERT_EVENTS_FaultReported_SIZE(0);
    ASSERT_EVENTS_FaultIgnored_SIZE(1);
    ASSERT_EVENTS_FaultIgnored(0, FATAL);
    ASSERT_TLM_FaultsIgnored(0, 1);
    this->clearHistory();

    // Still latched during the response
    this->tick(TICKS_TO_RESPONSE);
    this->assertDispatched(REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->report(FATAL);
    ASSERT_EVENTS_FaultIgnored_SIZE(1);
    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
}

void FaultManagerTester ::testInvalidReport() {
    const FaultConfig::Fault invalid(static_cast<FaultConfig::Fault::T>(FaultConfig::Fault::NUM_FAULTS));
    this->invoke_to_reportIn(0, invalid);
    ASSERT_EVENTS_FaultInvalid_SIZE(1);
    ASSERT_EVENTS_FaultInvalid(0, static_cast<U8>(FaultConfig::Fault::NUM_FAULTS));
    // Nothing was queued for the component thread
    ASSERT_EQ(this->component.m_queue.getMessagesAvailable(), 0);
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
}

void FaultManagerTester ::testFaultDisabled() {
    this->sendCommandSetFaultEnabled(FATAL, Fw::Enabled::DISABLED, Fw::CmdResponse::OK);
    ASSERT_EVENTS_FaultEnabledSet(0, FATAL, Fw::Enabled::DISABLED);
    this->clearHistory();

    this->report(FATAL);
    ASSERT_EVENTS_FaultReported_SIZE(0);
    ASSERT_EVENTS_FaultDisabled_SIZE(1);
    ASSERT_EVENTS_FaultDisabled(0, FATAL);
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
    this->clearHistory();

    // Disabled faults do not stay latched: re-enabling then reporting responds normally
    this->sendCommandSetFaultEnabled(FATAL, Fw::Enabled::ENABLED, Fw::CmdResponse::OK);
    this->clearHistory();
    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
}

void FaultManagerTester ::testStepFailureFault() {
    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_StepFailed_SIZE(1);
    ASSERT_EVENTS_StepFailed(0, REBOOT, REBOOT_RESPONSE, FATAL, FaultConfig::FailureMode::FAULT);
    ASSERT_EVENTS_ResponseFailed_SIZE(1);
    ASSERT_EVENTS_ResponseFailed(0, REBOOT_RESPONSE, FATAL);
    ASSERT_TLM_ResponsesFailed(0, 1);
    // The failure latched FAULT_RESPONSE_FAILURE, whose report is handled on the thread
    ASSERT_EVENTS_FaultReported_SIZE(1);
    ASSERT_EVENTS_FaultReported(0, FAILURE);
    this->clearHistory();

    // Response to the failure fault follows
    this->tick(TICKS_TO_RESPONSE);
    ASSERT_EVENTS_ResponseStarted(0, REBOOT_RESPONSE, FAILURE);
    this->assertDispatched(REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_ResponseCompleted(0, REBOOT_RESPONSE, FAILURE);
    this->clearHistory();

    // The original fault's latch was cleared by the failed response: idle
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
    ASSERT_EVENTS_SIZE(0);
}

void FaultManagerTester ::testStepFailureIgnore() {
    this->sendCommandUpdateStepFailureMode(REBOOT, FaultConfig::FailureMode::IGNORE, Fw::CmdResponse::OK);
    ASSERT_EVENTS_StepFailureModeSet(0, REBOOT, FaultConfig::FailureMode::IGNORE);
    this->clearHistory();

    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_StepFailed_SIZE(1);
    ASSERT_EVENTS_StepFailed(0, REBOOT, REBOOT_RESPONSE, FATAL, FaultConfig::FailureMode::IGNORE);
    ASSERT_EVENTS_ResponseFailed_SIZE(0);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
    ASSERT_EVENTS_FaultReported_SIZE(0);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
}

void FaultManagerTester ::testStepFailureDefer() {
    this->sendCommandUpdateStepFailureMode(REBOOT, FaultConfig::FailureMode::DEFER, Fw::CmdResponse::OK);
    this->clearHistory();

    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_StepFailed(0, REBOOT, REBOOT_RESPONSE, FATAL, FaultConfig::FailureMode::DEFER);
    // Remaining steps are SKIP, so the response ends and the deferred failure is applied
    ASSERT_EVENTS_ResponseFailed_SIZE(1);
    ASSERT_EVENTS_ResponseFailed(0, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_FaultReported(0, FAILURE);
}

void FaultManagerTester ::testResponseDisabled() {
    this->sendCommandSetResponseEnabled(REBOOT_RESPONSE, Fw::Enabled::DISABLED, Fw::CmdResponse::OK);
    ASSERT_EVENTS_ResponseEnabledSet(0, REBOOT_RESPONSE, Fw::Enabled::DISABLED);
    this->clearHistory();

    this->report(FATAL);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
    ASSERT_EVENTS_ResponseStarted_SIZE(1);
    ASSERT_EVENTS_StepSkipped_SIZE(1);
    ASSERT_EVENTS_StepSkipped(0, REBOOT, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
    this->clearHistory();

    // Latch cleared
    this->tick(TICKS_TO_RESPONSE);
    ASSERT_EVENTS_SIZE(0);
}

void FaultManagerTester ::testPreemption() {
    // FATAL (precedence 10) -> sequence response, so that FAULT_RESPONSE_FAILURE (precedence 20) can preempt it
    this->remapFault(FATAL, SEQUENCE_RESPONSE, 10);
    this->reportAndDispatch(FATAL, SEQUENCE_PORT, SEQUENCE_RESPONSE, RUN_SEQUENCE);

    this->report(FAILURE);
    ASSERT_EVENTS_FaultReported(0, FAILURE);
    ASSERT_EVENTS_StepCancel_SIZE(1);
    ASSERT_EVENTS_StepCancel(0, RUN_SEQUENCE);
    ASSERT_from_stepCancelOut_SIZE(1);
    ASSERT_EQ(this->m_last_cancel_port, static_cast<FwIndexType>(SEQUENCE_PORT.e));
    ASSERT_EVENTS_ResponsePreempted_SIZE(1);
    ASSERT_EVENTS_ResponsePreempted(0, SEQUENCE_RESPONSE, FATAL, FAILURE);
    ASSERT_EVENTS_ResponseCompleted_SIZE(0);
    ASSERT_EVENTS_ResponseFailed_SIZE(0);
    this->clearHistory();

    // Late completion of the canceled step is unexpected
    this->complete(Fw::Success::SUCCESS, SEQUENCE_RESPONSE, RUN_SEQUENCE);
    ASSERT_EVENTS_UnexpectedStepCompleted_SIZE(1);
    this->clearHistory();

    // The preempting fault is responded to after a fresh countdown
    this->tick(TICKS_TO_RESPONSE);
    ASSERT_EVENTS_ResponseStarted(0, REBOOT_RESPONSE, FAILURE);
    this->assertDispatched(REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_ResponseCompleted(0, REBOOT_RESPONSE, FAILURE);
    this->clearHistory();

    // The preempted fault stayed latched and is responded to next
    this->tick(TICKS_TO_RESPONSE);
    ASSERT_EVENTS_ResponseStarted(0, SEQUENCE_RESPONSE, FATAL);
    this->assertDispatched(SEQUENCE_PORT, SEQUENCE_RESPONSE, RUN_SEQUENCE);
    this->complete(Fw::Success::SUCCESS, SEQUENCE_RESPONSE, RUN_SEQUENCE);
    ASSERT_EVENTS_ResponseCompleted(0, SEQUENCE_RESPONSE, FATAL);
}

void FaultManagerTester ::testNoPreemptionLowerPrecedence() {
    this->remapFault(FATAL, SEQUENCE_RESPONSE, 10);
    // FAULT_RESPONSE_FAILURE (20) responds first; FATAL (10) arrives during the response
    this->reportAndDispatch(FAILURE, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->report(FATAL);
    ASSERT_EVENTS_FaultReported(0, FATAL);
    ASSERT_EVENTS_StepCancel_SIZE(0);
    ASSERT_EVENTS_ResponsePreempted_SIZE(0);
    this->clearHistory();

    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_ResponseCompleted(0, REBOOT_RESPONSE, FAILURE);
    this->clearHistory();

    this->tick(TICKS_TO_RESPONSE);
    ASSERT_EVENTS_ResponseStarted(0, SEQUENCE_RESPONSE, FATAL);
    this->assertDispatched(SEQUENCE_PORT, SEQUENCE_RESPONSE, RUN_SEQUENCE);
}

void FaultManagerTester ::testPrecedenceSelection() {
    this->remapFault(FATAL, SEQUENCE_RESPONSE, 10);
    this->report(FATAL);
    this->tick();
    this->report(FAILURE);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE - 1);
    ASSERT_EVENTS_ResponseStarted(0, REBOOT_RESPONSE, FAILURE);
    this->assertDispatched(REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
}

void FaultManagerTester ::testUnexpectedCompletion() {
    // Idle: any completion is unexpected
    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_UnexpectedStepCompleted_SIZE(1);
    ASSERT_EVENTS_UnexpectedStepCompleted(0, REBOOT, REBOOT_RESPONSE);
    this->clearHistory();

    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    // Wrong step and wrong response are both rejected and leave the response active
    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, RUN_SEQUENCE);
    this->complete(Fw::Success::SUCCESS, SEQUENCE_RESPONSE, REBOOT);
    ASSERT_EVENTS_UnexpectedStepCompleted_SIZE(2);
    ASSERT_EVENTS_StepCompleted_SIZE(0);
    this->clearHistory();

    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
}

void FaultManagerTester ::testCommandValidation() {
    const FaultConfig::Fault badFault(static_cast<FaultConfig::Fault::T>(FaultConfig::Fault::NUM_FAULTS));
    const FaultConfig::Response badResponse(
        static_cast<FaultConfig::Response::T>(FaultConfig::Response::NUM_RESPONSES));
    const FaultConfig::Step badStep(static_cast<FaultConfig::Step::T>(FaultConfig::Step::SKIP));

    this->sendCommandSetFaultEnabled(badFault, Fw::Enabled::DISABLED, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_InvalidFaultArgument_SIZE(1);
    ASSERT_EVENTS_InvalidFaultArgument(0, static_cast<U8>(FaultConfig::Fault::NUM_FAULTS));
    ASSERT_EVENTS_FaultEnabledSet_SIZE(0);
    this->sendCommandSetResponseEnabled(badResponse, Fw::Enabled::DISABLED, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_InvalidResponseArgument_SIZE(1);
    ASSERT_EVENTS_InvalidResponseArgument(0, static_cast<U8>(FaultConfig::Response::NUM_RESPONSES));
    ASSERT_EVENTS_ResponseEnabledSet_SIZE(0);
    this->sendCommandUpdateStepFailureMode(badStep, FaultConfig::FailureMode::IGNORE,
                                           Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_InvalidStepArgument_SIZE(1);
    ASSERT_EVENTS_InvalidStepArgument(0, static_cast<U8>(FaultConfig::Step::SKIP));
    ASSERT_EVENTS_StepFailureModeSet_SIZE(0);
    this->clearHistory();

    // Behavior unchanged by rejected commands
    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
}

void FaultManagerTester ::testResponseFailureNoRecursion() {
    this->reportAndDispatch(FAILURE, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_ResponseFailed_SIZE(1);
    ASSERT_EVENTS_ResponseFailed(0, REBOOT_RESPONSE, FAILURE);
    ASSERT_EVENTS_FaultReported_SIZE(0);
    ASSERT_EVENTS_FaultIgnored_SIZE(0);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
    ASSERT_EVENTS_SIZE(0);
}

void FaultManagerTester ::testMultiStepResponse() {
    this->remapFault(FATAL, SEQUENCE_THEN_REBOOT, 10);
    this->reportAndDispatch(FATAL, SEQUENCE_PORT, SEQUENCE_THEN_REBOOT, RUN_SEQUENCE);

    // Completing the first step dispatches the second to its own port; the response is not yet complete
    this->complete(Fw::Success::SUCCESS, SEQUENCE_THEN_REBOOT, RUN_SEQUENCE);
    ASSERT_EVENTS_StepCompleted_SIZE(1);
    ASSERT_EVENTS_StepCompleted(0, RUN_SEQUENCE, SEQUENCE_THEN_REBOOT, FATAL);
    ASSERT_EVENTS_StepStarted_SIZE(1);
    ASSERT_EVENTS_StepStarted(0, REBOOT, SEQUENCE_THEN_REBOOT, FATAL);
    ASSERT_EVENTS_ResponseCompleted_SIZE(0);
    this->assertDispatched(REBOOT_PORT, SEQUENCE_THEN_REBOOT, REBOOT);
    this->clearHistory();

    // A completion for the already finished step is unexpected while the second step is active
    this->complete(Fw::Success::SUCCESS, SEQUENCE_THEN_REBOOT, RUN_SEQUENCE);
    ASSERT_EVENTS_UnexpectedStepCompleted_SIZE(1);
    ASSERT_EVENTS_ResponseCompleted_SIZE(0);
    this->clearHistory();

    // The trailing SKIP ends the response after the second step completes
    this->complete(Fw::Success::SUCCESS, SEQUENCE_THEN_REBOOT, REBOOT);
    ASSERT_EVENTS_StepCompleted(0, REBOOT, SEQUENCE_THEN_REBOOT, FATAL);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
    ASSERT_EVENTS_ResponseCompleted(0, SEQUENCE_THEN_REBOOT, FATAL);
    ASSERT_TLM_ResponsesCompleted(0, 1);
    this->assertNotDispatched();
}

void FaultManagerTester ::testDeferThenContinue() {
    this->remapFault(FATAL, SEQUENCE_THEN_REBOOT, 10);
    this->sendCommandUpdateStepFailureMode(RUN_SEQUENCE, FaultConfig::FailureMode::DEFER, Fw::CmdResponse::OK);
    this->clearHistory();
    this->reportAndDispatch(FATAL, SEQUENCE_PORT, SEQUENCE_THEN_REBOOT, RUN_SEQUENCE);

    // DEFER: the failure is recorded, but the remaining steps still run
    this->complete(Fw::Success::FAILURE, SEQUENCE_THEN_REBOOT, RUN_SEQUENCE);
    ASSERT_EVENTS_StepFailed_SIZE(1);
    ASSERT_EVENTS_StepFailed(0, RUN_SEQUENCE, SEQUENCE_THEN_REBOOT, FATAL, FaultConfig::FailureMode::DEFER);
    ASSERT_EVENTS_ResponseFailed_SIZE(0);
    ASSERT_EVENTS_FaultReported_SIZE(0);
    ASSERT_EVENTS_StepStarted(0, REBOOT, SEQUENCE_THEN_REBOOT, FATAL);
    this->assertDispatched(REBOOT_PORT, SEQUENCE_THEN_REBOOT, REBOOT);
    this->clearHistory();

    // A successful final step cannot rescue the deferred failure
    this->complete(Fw::Success::SUCCESS, SEQUENCE_THEN_REBOOT, REBOOT);
    ASSERT_EVENTS_StepCompleted_SIZE(1);
    ASSERT_EVENTS_ResponseCompleted_SIZE(0);
    ASSERT_EVENTS_ResponseFailed_SIZE(1);
    ASSERT_EVENTS_ResponseFailed(0, SEQUENCE_THEN_REBOOT, FATAL);
    ASSERT_TLM_ResponsesFailed(0, 1);
    ASSERT_EVENTS_FaultReported_SIZE(1);
    ASSERT_EVENTS_FaultReported(0, FAILURE);
}

void FaultManagerTester ::testStepTimeout() {
    const StepDefinitionTable steps;
    FwSizeType timeout = 0;
    for (FwSizeType i = 0; i < StepDefinitionTable::SIZE; i++) {
        if (steps[i].get_step() == REBOOT) {
            timeout = steps[i].get_timeoutTicks();
        }
    }
    ASSERT_GT(timeout, 0) << "test requires a REBOOT step timeout";

    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);

    // The step is awaited for timeoutTicks - 1 ticks without consequence
    this->tick(timeout - 1);
    ASSERT_EVENTS_StepTimedOut_SIZE(0);
    ASSERT_EVENTS_SIZE(0);

    // The expiring tick cancels the step and fails it per its failure mode (FAULT)
    this->tick();
    ASSERT_EVENTS_StepTimedOut_SIZE(1);
    ASSERT_EVENTS_StepTimedOut(0, REBOOT, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_StepCancel_SIZE(1);
    ASSERT_EVENTS_StepCancel(0, REBOOT);
    ASSERT_from_stepCancelOut_SIZE(1);
    ASSERT_EQ(this->m_last_cancel_port, static_cast<FwIndexType>(REBOOT_PORT.e));
    ASSERT_EVENTS_StepFailed_SIZE(1);
    ASSERT_EVENTS_StepFailed(0, REBOOT, REBOOT_RESPONSE, FATAL, FaultConfig::FailureMode::FAULT);
    ASSERT_EVENTS_ResponseFailed_SIZE(1);
    ASSERT_EVENTS_ResponseFailed(0, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_FaultReported(0, FAILURE);
    this->clearHistory();

    // A late completion of the timed-out step is unexpected
    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_UnexpectedStepCompleted_SIZE(1);
    this->clearHistory();

    // A fresh dispatch starts a fresh timeout. FAULT_RESPONSE_FAILURE was latched during the expiring tick, so
    // that tick already counted toward its countdown.
    this->tick(TICKS_TO_RESPONSE - 1);
    this->assertDispatched(REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->clearHistory();
    this->tick(timeout - 1);
    ASSERT_EVENTS_StepTimedOut_SIZE(0);
    this->complete(Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_ResponseCompleted(0, REBOOT_RESPONSE, FAILURE);
}

void FaultManagerTester ::testStepPortUnconnected() {
    // A second instance with REBOOT_RESPONDER_PORT left unconnected; events and telemetry route to this tester
    FaultManager bare("FaultManagerBare");
    bare.init(FaultManagerTester::TEST_INSTANCE_QUEUE_DEPTH, FaultManagerTester::TEST_INSTANCE_ID + 1);
    bare.set_logOut_OutputPort(0, this->get_from_logOut(0));
#if FW_ENABLE_TEXT_LOGGING == 1
    bare.set_logTextOut_OutputPort(0, this->get_from_logTextOut(0));
#endif
    bare.set_timeCaller_OutputPort(0, this->get_from_timeCaller(0));
    bare.set_tlmOut_OutputPort(0, this->get_from_tlmOut(0));
    bare.set_stepDispatchOut_OutputPort(static_cast<FwIndexType>(SEQUENCE_PORT.e),
                                        this->get_from_stepDispatchOut(static_cast<FwIndexType>(SEQUENCE_PORT.e)));
    bare.set_stepCancelOut_OutputPort(static_cast<FwIndexType>(SEQUENCE_PORT.e),
                                      this->get_from_stepCancelOut(static_cast<FwIndexType>(SEQUENCE_PORT.e)));
    ASSERT_FALSE(bare.isConnected_stepDispatchOut_OutputPort(static_cast<FwIndexType>(REBOOT_PORT.e)));
    this->clearHistory();

    bare.get_reportIn_InputPort(0)->invoke(FATAL);
    this->dispatchAll(bare);
    ASSERT_EVENTS_FaultReported_SIZE(1);
    this->clearHistory();
    for (FwSizeType i = 0; i < TICKS_TO_RESPONSE; i++) {
        bare.get_run_InputPort(0)->invoke(0);
        this->dispatchAll(bare);
    }

    // The step cannot be dispatched: it fails without a port call and the response fails per the step's mode
    ASSERT_EVENTS_ResponseStarted(0, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_StepStarted(0, REBOOT, REBOOT_RESPONSE, FATAL);
    ASSERT_EVENTS_StepPortUnconnected_SIZE(1);
    ASSERT_EVENTS_StepPortUnconnected(0, REBOOT, REBOOT_PORT);
    this->assertNotDispatched();
    ASSERT_from_stepCancelOut_SIZE(0);
    ASSERT_EVENTS_StepFailed_SIZE(1);
    ASSERT_EVENTS_StepFailed(0, REBOOT, REBOOT_RESPONSE, FATAL, FaultConfig::FailureMode::FAULT);
    ASSERT_EVENTS_ResponseFailed_SIZE(1);
    ASSERT_EVENTS_FaultReported(0, FAILURE);
    bare.deinit();
}

void FaultManagerTester ::testIgnoredReportThrottle() {
    this->report(FATAL);
    this->clearHistory();

    // Reports beyond the throttle are latched-and-ignored silently
    const FwSizeType throttle = FaultManagerComponentBase::EVENTID_FAULTIGNORED_THROTTLE;
    for (FwSizeType i = 0; i < throttle + 2; i++) {
        this->report(FATAL);
    }
    ASSERT_EVENTS_FaultIgnored_SIZE(throttle);
    this->clearHistory();

    // Each run tick clears the throttle
    this->tick();
    this->report(FATAL);
    ASSERT_EVENTS_FaultIgnored_SIZE(1);
    ASSERT_EVENTS_FaultIgnored(0, FATAL);
    this->clearHistory();

    // FaultInvalid is throttled and cleared the same way
    const FaultConfig::Fault badFault(static_cast<FaultConfig::Fault::T>(FaultConfig::Fault::NUM_FAULTS));
    const FwSizeType invalidThrottle = FaultManagerComponentBase::EVENTID_FAULTINVALID_THROTTLE;
    for (FwSizeType i = 0; i < invalidThrottle + 2; i++) {
        this->report(badFault);
    }
    ASSERT_EVENTS_FaultInvalid_SIZE(invalidThrottle);
    this->clearHistory();
    this->tick();
    this->report(badFault);
    ASSERT_EVENTS_FaultInvalid_SIZE(1);
}

void FaultManagerTester ::testDisableClearsLatch() {
    this->report(FATAL);
    ASSERT_EVENTS_FaultReported_SIZE(1);
    this->clearHistory();

    // Disabling a latched fault discards its pending report
    this->sendCommandSetFaultEnabled(FATAL, Fw::Enabled::DISABLED, Fw::CmdResponse::OK);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
    ASSERT_EVENTS_ResponseStarted_SIZE(0);

    // Re-enabling does not resurrect the discarded report
    this->sendCommandSetFaultEnabled(FATAL, Fw::Enabled::ENABLED, Fw::CmdResponse::OK);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
    ASSERT_EVENTS_SIZE(0);

    // A fresh report is accepted (the latch was cleared) and responded to
    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
}

void FaultManagerTester ::testTableParameters() {
    // Valid RESPONSE_TABLE and STEP_TABLE parameters override the FaultConfig defaults
    ResponsesEnabled responses;
    responses[REBOOT_RESPONSE.e] = Fw::Enabled::DISABLED;
    StepFailureModes steps;
    steps[RUN_SEQUENCE.e] = FaultConfig::FailureMode::IGNORE;
    steps[REBOOT.e] = FaultConfig::FailureMode::IGNORE;
    this->paramSet_RESPONSE_TABLE(responses, Fw::ParamValid::VALID);
    this->paramSet_STEP_TABLE(steps, Fw::ParamValid::VALID);
    this->component.loadParameters();
    this->clearHistory();

    // REBOOT_RESPONSE disabled by parameter: its step is skipped
    this->report(FATAL);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE);
    this->assertNotDispatched();
    ASSERT_EVENTS_StepSkipped_SIZE(1);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
    this->clearHistory();

    // REBOOT failure mode IGNORE by parameter: a failed step completes the response
    this->sendCommandSetResponseEnabled(REBOOT_RESPONSE, Fw::Enabled::ENABLED, Fw::CmdResponse::OK);
    this->clearHistory();
    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_StepFailed(0, REBOOT, REBOOT_RESPONSE, FATAL, FaultConfig::FailureMode::IGNORE);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
    ASSERT_EVENTS_FaultReported_SIZE(0);
    this->clearHistory();

    // Invalid parameters leave the cached tables untouched
    this->paramSet_RESPONSE_TABLE(responses, Fw::ParamValid::INVALID);
    this->paramSet_STEP_TABLE(steps, Fw::ParamValid::INVALID);
    this->component.loadParameters();
    this->clearHistory();
    this->reportAndDispatch(FATAL, REBOOT_PORT, REBOOT_RESPONSE, REBOOT);
    this->complete(Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    ASSERT_EVENTS_StepFailed(0, REBOOT, REBOOT_RESPONSE, FATAL, FaultConfig::FailureMode::IGNORE);
    ASSERT_EVENTS_ResponseCompleted_SIZE(1);
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

void FaultManagerTester ::from_stepDispatchOut_handler(FwIndexType portNum,
                                                       const FaultConfig::Response& response,
                                                       const FaultConfig::Step& step,
                                                       const FaultConfig::Context& context) {
    this->m_last_dispatch_port = portNum;
    this->pushFromPortEntry_stepDispatchOut(response, step, context);
}

void FaultManagerTester ::from_stepCancelOut_handler(FwIndexType portNum) {
    this->m_last_cancel_port = portNum;
    this->pushFromPortEntry_stepCancelOut();
}

void FaultManagerTester ::textLogIn(FwEventIdType id,
                                    const Fw::Time& timeTag,
                                    const Fw::LogSeverity severity,
                                    const Fw::TextLogString& text) {
    TextLogEntry e = {id, timeTag, severity, text};
    printTextLogHistoryEntry(e, stdout);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void FaultManagerTester ::dispatchAll(FaultManager& target) {
    // Bounded: each dispatched message queues at most one further message and a response has finitely many steps
    for (FwSizeType i = 0; i < TEST_INSTANCE_QUEUE_DEPTH; i++) {
        if (target.m_queue.getMessagesAvailable() == 0) {
            return;
        }
        ASSERT_EQ(target.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    }
    ASSERT_EQ(target.m_queue.getMessagesAvailable(), 0);
}

void FaultManagerTester ::report(const FaultConfig::Fault& fault) {
    this->invoke_to_reportIn(0, fault);
    this->dispatchAll(this->component);
}

void FaultManagerTester ::tick(FwSizeType count) {
    for (FwSizeType i = 0; i < count; i++) {
        this->invoke_to_run(0, 0);
        this->dispatchAll(this->component);
    }
}

void FaultManagerTester ::complete(const Fw::Success& status,
                                   const FaultConfig::Response& response,
                                   const FaultConfig::Step& step) {
    this->invoke_to_stepCompletionIn(0, status, response, step);
    this->dispatchAll(this->component);
}

void FaultManagerTester ::assertDispatched(const FaultConfig::Port& port,
                                           const FaultConfig::Response& response,
                                           const FaultConfig::Step& step) {
    ASSERT_EQ(this->fromPortHistory_stepDispatchOut->size(), 1) << "expected exactly one step dispatch";
    ASSERT_EQ(this->fromPortHistory_stepDispatchOut->at(0).response, response);
    ASSERT_EQ(this->fromPortHistory_stepDispatchOut->at(0).step, step);
    ASSERT_EQ(this->m_last_dispatch_port, static_cast<FwIndexType>(port.e));
    this->clearFromPortHistory();
}

void FaultManagerTester ::assertNotDispatched() {
    ASSERT_from_stepDispatchOut_SIZE(0);
}

void FaultManagerTester ::reportAndDispatch(const FaultConfig::Fault& fault,
                                            const FaultConfig::Port& port,
                                            const FaultConfig::Response& response,
                                            const FaultConfig::Step& step) {
    this->report(fault);
    ASSERT_EVENTS_FaultReported_SIZE(1);
    this->clearHistory();
    this->tick(TICKS_TO_RESPONSE);
    ASSERT_EVENTS_ResponseStarted_SIZE(1);
    ASSERT_EVENTS_ResponseStarted(0, response, fault);
    this->assertDispatched(port, response, step);
    this->clearHistory();
}

void FaultManagerTester ::remapFault(const FaultConfig::Fault& fault,
                                     const FaultConfig::Response& response,
                                     U8 precedence) {
    FaultResponseTable table;
    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        if (table[i].get_fault() == fault) {
            table[i].set_response(response);
            table[i].set_precedence(precedence);
        }
    }
    this->paramSet_FAULT_RESPONSE_TABLE(table, Fw::ParamValid::VALID);
    this->component.loadParameters();
    this->clearHistory();
}

void FaultManagerTester ::sendCommandSetFaultEnabled(const FaultConfig::Fault& fault,
                                                     const Fw::Enabled& enabled,
                                                     const Fw::CmdResponse& expected) {
    this->clearHistory();
    this->sendCmd_SET_FAULT_ENABLED(0, 1, fault, enabled);
    this->dispatchAll(this->component);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, FaultManager::OPCODE_SET_FAULT_ENABLED, 1, expected);
}

void FaultManagerTester ::sendCommandSetResponseEnabled(const FaultConfig::Response& response,
                                                        const Fw::Enabled& enabled,
                                                        const Fw::CmdResponse& expected) {
    this->clearHistory();
    this->sendCmd_SET_RESPONSE_ENABLED(0, 2, response, enabled);
    this->dispatchAll(this->component);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, FaultManager::OPCODE_SET_RESPONSE_ENABLED, 2, expected);
}

void FaultManagerTester ::sendCommandUpdateStepFailureMode(const FaultConfig::Step& step,
                                                           const FaultConfig::FailureMode& mode,
                                                           const Fw::CmdResponse& expected) {
    this->clearHistory();
    this->sendCmd_UPDATE_STEP_FAILURE_MODE(0, 3, step, mode);
    this->dispatchAll(this->component);
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, FaultManager::OPCODE_UPDATE_STEP_FAILURE_MODE, 3, expected);
}

}  // namespace FaultProtection

}  // namespace Svc
