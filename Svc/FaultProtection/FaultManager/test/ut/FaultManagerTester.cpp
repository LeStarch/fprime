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
      m_last_dispatch_port(-1) {
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
    ASSERT_EVENTS_InvalidCommandArgument_SIZE(1);
    ASSERT_EVENTS_InvalidCommandArgument(0, static_cast<U8>(FaultConfig::Fault::NUM_FAULTS));
    ASSERT_EVENTS_FaultEnabledSet_SIZE(0);
    this->sendCommandSetResponseEnabled(badResponse, Fw::Enabled::DISABLED, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_InvalidCommandArgument_SIZE(1);
    ASSERT_EVENTS_InvalidCommandArgument(0, static_cast<U8>(FaultConfig::Response::NUM_RESPONSES));
    ASSERT_EVENTS_ResponseEnabledSet_SIZE(0);
    this->sendCommandUpdateStepFailureMode(badStep, FaultConfig::FailureMode::IGNORE,
                                           Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_InvalidCommandArgument_SIZE(1);
    ASSERT_EVENTS_InvalidCommandArgument(0, static_cast<U8>(FaultConfig::Step::SKIP));
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

void FaultManagerTester ::dispatchAll() {
    // Bounded: each dispatched message queues at most one further message and a response has finitely many steps
    for (FwSizeType i = 0; i < TEST_INSTANCE_QUEUE_DEPTH; i++) {
        if (this->component.m_queue.getMessagesAvailable() == 0) {
            return;
        }
        ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    }
    ASSERT_EQ(this->component.m_queue.getMessagesAvailable(), 0);
}

void FaultManagerTester ::report(const FaultConfig::Fault& fault) {
    this->invoke_to_reportIn(0, fault);
    this->dispatchAll();
}

void FaultManagerTester ::tick(FwSizeType count) {
    for (FwSizeType i = 0; i < count; i++) {
        this->invoke_to_run(0, 0);
        this->dispatchAll();
    }
}

void FaultManagerTester ::complete(const Fw::Success& status,
                                   const FaultConfig::Response& response,
                                   const FaultConfig::Step& step) {
    this->invoke_to_stepCompletionIn(0, status, response, step);
    this->dispatchAll();
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
    this->dispatchAll();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, FaultManager::OPCODE_SET_FAULT_ENABLED, 1, expected);
}

void FaultManagerTester ::sendCommandSetResponseEnabled(const FaultConfig::Response& response,
                                                        const Fw::Enabled& enabled,
                                                        const Fw::CmdResponse& expected) {
    this->clearHistory();
    this->sendCmd_SET_RESPONSE_ENABLED(0, 2, response, enabled);
    this->dispatchAll();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, FaultManager::OPCODE_SET_RESPONSE_ENABLED, 2, expected);
}

void FaultManagerTester ::sendCommandUpdateStepFailureMode(const FaultConfig::Step& step,
                                                           const FaultConfig::FailureMode& mode,
                                                           const Fw::CmdResponse& expected) {
    this->clearHistory();
    this->sendCmd_UPDATE_STEP_FAILURE_MODE(0, 3, step, mode);
    this->dispatchAll();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, FaultManager::OPCODE_UPDATE_STEP_FAILURE_MODE, 3, expected);
}

}  // namespace FaultProtection

}  // namespace Svc
