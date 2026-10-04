// ======================================================================
// \title  SequenceResponderTester.cpp
// \author mstarch
// \brief  cpp file for SequenceResponder component test harness implementation class
// ======================================================================

#include "SequenceResponderTester.hpp"

namespace Svc {

namespace FaultProtection {

namespace {
const char* const SEQUENCE_DIRECTORY = "/fault/sequences";
const FaultConfig::Response SEQUENCE_RESPONSE(FaultConfig::Response::SEQUENCE_RESPONSE);
const FaultConfig::Response REBOOT_RESPONSE(FaultConfig::Response::REBOOT_RESPONSE);
const FaultConfig::Step RUN_SEQUENCE(FaultConfig::Step::RUN_SEQUENCE);
const FaultConfig::Step REBOOT(FaultConfig::Step::REBOOT);
#if FW_SERIALIZABLE_TO_STRING
const char* const RUN_SEQUENCE_FILE = "/fault/sequences/RUN_SEQUENCE.seq";
const char* const REBOOT_FILE = "/fault/sequences/REBOOT.seq";
#else
const char* const RUN_SEQUENCE_FILE = "/fault/sequences/0.seq";
const char* const REBOOT_FILE = "/fault/sequences/1.seq";
#endif
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

SequenceResponderTester ::SequenceResponderTester()
    : SequenceResponderGTestBase("SequenceResponderTester", SequenceResponderTester::MAX_HISTORY_SIZE),
      component("SequenceResponder") {
    this->initComponents();
    this->connectPorts();
    this->component.configure(SEQUENCE_DIRECTORY);
}

SequenceResponderTester ::~SequenceResponderTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void SequenceResponderTester ::testNominalSequence() {
    this->dispatch(SEQUENCE_RESPONSE, RUN_SEQUENCE, RUN_SEQUENCE_FILE);
    ASSERT_from_faultResponseComplete_SIZE(0);
    this->sequenceDone(Fw::CmdResponse::OK);
    ASSERT_EVENTS_SequenceCompleted_SIZE(1);
    ASSERT_EVENTS_SequenceCompleted(0, RUN_SEQUENCE, SEQUENCE_RESPONSE);
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::SUCCESS, SEQUENCE_RESPONSE, RUN_SEQUENCE);
}

void SequenceResponderTester ::testConsecutiveSteps() {
    this->dispatch(SEQUENCE_RESPONSE, RUN_SEQUENCE, RUN_SEQUENCE_FILE);
    this->sequenceDone(Fw::CmdResponse::OK);
    ASSERT_from_faultResponseComplete(0, Fw::Success::SUCCESS, SEQUENCE_RESPONSE, RUN_SEQUENCE);
    this->clearHistory();
    // The file name follows the step, not the response
    this->dispatch(REBOOT_RESPONSE, REBOOT, REBOOT_FILE);
    this->sequenceDone(Fw::CmdResponse::OK);
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::SUCCESS, REBOOT_RESPONSE, REBOOT);
}

void SequenceResponderTester ::testSequenceFailure() {
    this->dispatch(SEQUENCE_RESPONSE, RUN_SEQUENCE, RUN_SEQUENCE_FILE);
    this->sequenceDone(Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_SequenceFailed_SIZE(1);
    ASSERT_EVENTS_SequenceFailed(0, RUN_SEQUENCE, SEQUENCE_RESPONSE, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_SequenceCompleted_SIZE(0);
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::FAILURE, SEQUENCE_RESPONSE, RUN_SEQUENCE);
}

void SequenceResponderTester ::testBusy() {
    this->dispatch(SEQUENCE_RESPONSE, RUN_SEQUENCE, RUN_SEQUENCE_FILE);
    this->invoke_to_faultResponseDispatch(0, REBOOT_RESPONSE, REBOOT, FaultConfig::Context());
    // The second step is rejected without disturbing the running sequence
    ASSERT_EVENTS_ResponderBusy_SIZE(1);
    ASSERT_EVENTS_ResponderBusy(0, REBOOT, REBOOT_RESPONSE);
    // Only the first step's sequence was requested
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    this->clearHistory();
    // The first step still completes with its sequence
    this->sequenceDone(Fw::CmdResponse::OK);
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::SUCCESS, SEQUENCE_RESPONSE, RUN_SEQUENCE);
}

void SequenceResponderTester ::testCancel() {
    this->dispatch(SEQUENCE_RESPONSE, RUN_SEQUENCE, RUN_SEQUENCE_FILE);
    this->invoke_to_faultResponseCancel(0);
    ASSERT_EVENTS_SequenceCanceled_SIZE(1);
    ASSERT_EVENTS_SequenceCanceled(0, RUN_SEQUENCE, SEQUENCE_RESPONSE);
    ASSERT_from_seqCancelOut_SIZE(1);
    ASSERT_from_faultResponseComplete_SIZE(0);
    this->clearHistory();
    // The sequencer's cancellation status is not forwarded as a step completion
    this->sequenceDone(Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_UnexpectedSequenceDone_SIZE(1);
    ASSERT_from_faultResponseComplete_SIZE(0);
    this->clearHistory();
    // The responder is free for the next step
    this->dispatch(REBOOT_RESPONSE, REBOOT, REBOOT_FILE);
}

void SequenceResponderTester ::testIdleInputs() {
    this->invoke_to_faultResponseCancel(0);
    ASSERT_EVENTS_SIZE(0);
    ASSERT_from_seqCancelOut_SIZE(0);
    this->sequenceDone(Fw::CmdResponse::OK);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_UnexpectedSequenceDone_SIZE(1);
    ASSERT_EVENTS_UnexpectedSequenceDone(0, Fw::CmdResponse::OK);
    ASSERT_from_faultResponseComplete_SIZE(0);
}

void SequenceResponderTester ::testUnconnectedSequencer() {
    SequenceResponder bare("bare");
    bare.init(TEST_INSTANCE_ID);
    bare.set_logOut_OutputPort(0, this->get_from_logOut(0));
#if FW_ENABLE_TEXT_LOGGING == 1
    bare.set_logTextOut_OutputPort(0, this->get_from_logTextOut(0));
#endif
    bare.set_timeCaller_OutputPort(0, this->get_from_timeCaller(0));
    bare.set_faultResponseComplete_OutputPort(0, this->get_from_faultResponseComplete(0));
    bare.get_faultResponseDispatch_InputPort(0)->invoke(SEQUENCE_RESPONSE, RUN_SEQUENCE, FaultConfig::Context());
    ASSERT_EVENTS_SequencerUnconnected_SIZE(1);
    ASSERT_EVENTS_SequenceStarted_SIZE(0);
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::FAILURE, SEQUENCE_RESPONSE, RUN_SEQUENCE);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void SequenceResponderTester ::dispatch(const FaultConfig::Response& response,
                                        const FaultConfig::Step& step,
                                        const char* expectedFile) {
    this->invoke_to_faultResponseDispatch(0, response, step, FaultConfig::Context());
    ASSERT_EVENTS_SequenceStarted_SIZE(1);
    ASSERT_EVENTS_SequenceStarted(0, step, response, expectedFile);
    ASSERT_from_seqRunOut_SIZE(1);
    ASSERT_STREQ(this->fromPortHistory_seqRunOut->at(0).filename.toChar(), expectedFile);
}

void SequenceResponderTester ::sequenceDone(const Fw::CmdResponse& status) {
    this->invoke_to_seqDoneIn(0, 0, 0, status);
}

}  // namespace FaultProtection

}  // namespace Svc
