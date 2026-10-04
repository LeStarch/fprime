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
    // No downlink to wait for under test
    this->component.configure(Fw::TimeInterval(0, 0));
}

RebootResponderTester ::~RebootResponderTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void RebootResponderTester ::testDispatchReboots() {
    this->invoke_to_faultResponseDispatch(0, REBOOT_RESPONSE, REBOOT, FaultConfig::Context());
    ASSERT_EVENTS_RebootRequested_SIZE(1);
    ASSERT_EVENTS_RebootRequested(0, REBOOT_RESPONSE, REBOOT);
    ASSERT_EQ(this->component.rebootCount, 1);
}

void RebootResponderTester ::testHookReturnFails() {
    this->invoke_to_faultResponseDispatch(0, REBOOT_RESPONSE, REBOOT, FaultConfig::Context());
    // The hook returned instead of rebooting: the step is reported as failed so FaultManager can escalate
    ASSERT_from_faultResponseComplete_SIZE(1);
    ASSERT_from_faultResponseComplete(0, Fw::Success::FAILURE, REBOOT_RESPONSE, REBOOT);
}

void RebootResponderTester ::testCancelRefused() {
    this->invoke_to_faultResponseCancel(0);
    ASSERT_EVENTS_SIZE(1);
    ASSERT_EVENTS_RebootCancelRefused_SIZE(1);
    ASSERT_from_faultResponseComplete_SIZE(0);
    ASSERT_EQ(this->component.rebootCount, 0);
}

}  // namespace FaultProtection

}  // namespace Svc
