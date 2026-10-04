// ======================================================================
// \title  FatalToFaultTester.cpp
// \author mstarch
// \brief  cpp file for FatalToFault component test harness implementation class
// ======================================================================

#include "FatalToFaultTester.hpp"

namespace Svc {

namespace FaultProtection {

namespace {
const FwEventIdType SOME_FATAL_ID = 0x1234;
const FaultConfig::Fault FATAL_OCCURRED(FaultConfig::Fault::FATAL_OCCURRED);
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

FatalToFaultTester ::FatalToFaultTester()
    : FatalToFaultGTestBase("FatalToFaultTester", FatalToFaultTester::MAX_HISTORY_SIZE), component("FatalToFault") {
    this->initComponents();
    this->connectPorts();
    this->component.configure(FALLBACK_TICKS);
}

FatalToFaultTester ::~FatalToFaultTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void FatalToFaultTester ::testFatalReportsFault() {
    this->invoke_to_FatalReceive(0, SOME_FATAL_ID);
    ASSERT_from_faultOut_SIZE(1);
    ASSERT_from_faultOut(0, FATAL_OCCURRED);
    ASSERT_EQ(this->component.fallbackCount, 0);
}

void FatalToFaultTester ::testTicksWithoutFatal() {
    this->tick(FALLBACK_TICKS * 10);
    ASSERT_from_faultOut_SIZE(0);
    ASSERT_EQ(this->component.fallbackCount, 0);
}

void FatalToFaultTester ::testFallbackCountdown() {
    this->invoke_to_FatalReceive(0, SOME_FATAL_ID);
    this->tick(FALLBACK_TICKS - 1);
    ASSERT_EQ(this->component.fallbackCount, 0);
    this->tick(1);
    ASSERT_EQ(this->component.fallbackCount, 1);
    // Still running (the test fallback does not end the process): the fallback keeps being requested
    this->tick(1);
    ASSERT_EQ(this->component.fallbackCount, 2);
}

void FatalToFaultTester ::testRepeatedFatal() {
    this->invoke_to_FatalReceive(0, SOME_FATAL_ID);
    this->tick(FALLBACK_TICKS - 1);
    this->invoke_to_FatalReceive(0, SOME_FATAL_ID + 1);
    // FaultManager latches the fault; every FATAL is still forwarded so the report cannot be lost
    ASSERT_from_faultOut_SIZE(2);
    ASSERT_from_faultOut(1, FATAL_OCCURRED);
    // The countdown runs from the first FATAL
    this->tick(1);
    ASSERT_EQ(this->component.fallbackCount, 1);
}

void FatalToFaultTester ::testUnconnectedFallback() {
    TestFatalToFault bare("bare");
    bare.init(TEST_INSTANCE_ID);
    bare.get_FatalReceive_InputPort(0)->invoke(SOME_FATAL_ID);
    ASSERT_EQ(bare.fallbackCount, 1);
    ASSERT_from_faultOut_SIZE(0);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void FatalToFaultTester ::tick(FwSizeType count) {
    for (FwSizeType i = 0; i < count; i++) {
        this->invoke_to_run(0, 0);
    }
}

}  // namespace FaultProtection

}  // namespace Svc
