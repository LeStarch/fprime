// ======================================================================
// \title  RebootResponderTester.cpp
// \author mstarch
// \brief  cpp file for RebootResponder component test harness implementation class
// ======================================================================

#include "RebootResponderTester.hpp"

namespace Svc {

namespace FaultProtection {

namespace {
const FaultConfig::Response REBOOT_RESPONSE(FaultConfig::Response::REBOOT_RESPONSE);
const FaultConfig::Step REBOOT(FaultConfig::Step::REBOOT);
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

RebootResponderTester ::RebootResponderTester()
    : RebootResponderGTestBase("RebootResponderTester", RebootResponderTester::MAX_HISTORY_SIZE),
      component("RebootResponder") {
    this->initComponents();
    this->connectPorts();
    this->component.configure(REBOOT_DELAY_TICKS);
}

RebootResponderTester ::~RebootResponderTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void RebootResponderTester ::testDispatchReboots() {
    this->invoke_to_faultResponseDispatch(0, REBOOT_RESPONSE, REBOOT, FaultConfig::Context());
    // Announced immediately, performed only once the delay has elapsed (the dispatch does not block)
    ASSERT_EVENTS_RebootRequested_SIZE(1);
    ASSERT_EVENTS_RebootRequested(0, REBOOT_RESPONSE, REBOOT);
    ASSERT_EQ(this->component.rebootCount, 0);
    this->tick(REBOOT_DELAY_TICKS - 1);
    ASSERT_EQ(this->component.rebootCount, 0);
    this->tick(1);
    ASSERT_EQ(this->component.rebootCount, 1);
}

void RebootResponderTester ::testTicksWithoutRequest() {
    this->tick(REBOOT_DELAY_TICKS * 10);
    ASSERT_EQ(this->component.rebootCount, 0);
    ASSERT_EVENTS_SIZE(0);
    ASSERT_from_faultResponseComplete_SIZE(0);
}

void RebootResponderTester ::testRepeatedRequest() {
    this->invoke_to_faultResponseDispatch(0, REBOOT_RESPONSE, REBOOT, FaultConfig::Context());
    this->tick(REBOOT_DELAY_TICKS - 1);
    const FaultConfig::Response otherResponse(FaultConfig::Response::SEQUENCE_THEN_REBOOT_RESPONSE);
    this->invoke_to_faultResponseDispatch(0, otherResponse, REBOOT, FaultConfig::Context());
    ASSERT_EVENTS_RebootRequested_SIZE(2);
    ASSERT_EVENTS_RebootRequested(1, otherResponse, REBOOT);
    // The delay runs from the first request, which stays authoritative for the completion
    this->tick(1);
    ASSERT_EQ(this->component.rebootCount, 1);
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
}

void RebootResponderTester ::testHookReturnFails() {
    this->invoke_to_faultResponseDispatch(0, REBOOT_RESPONSE, REBOOT, FaultConfig::Context());
    this->tick(REBOOT_DELAY_TICKS);
    // The hook returned instead of rebooting: the step is reported as failed so FaultManager can escalate
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
    // Nothing remains pending
    this->tick(REBOOT_DELAY_TICKS);
    ASSERT_EQ(this->component.rebootCount, 1);
    ASSERT_from_faultResponseComplete_SIZE(1);
}

void RebootResponderTester ::testCancelRefused() {
    this->invoke_to_faultResponseDispatch(0, REBOOT_RESPONSE, REBOOT, FaultConfig::Context());
    this->invoke_to_faultResponseCancel(0);
    ASSERT_EVENTS_RebootCancelRefused_SIZE(1);
    ASSERT_from_faultResponseComplete_SIZE(0);
    // The reboot proceeds regardless
    this->tick(REBOOT_DELAY_TICKS);
    ASSERT_EQ(this->component.rebootCount, 1);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void RebootResponderTester ::tick(FwSizeType count) {
    for (FwSizeType i = 0; i < count; i++) {
        this->invoke_to_run(0, 0);
    }
}

}  // namespace FaultProtection

}  // namespace Svc
