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
    this->component.faultsReported = 0;
    this->component.configure(Fw::TimeInterval(0, FALLBACK_DELAY_USECONDS));
}

FatalToFaultTester ::~FatalToFaultTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void FatalToFaultTester ::testFatalReportsFaultThenFallsBack() {
    this->invoke_to_FatalReceive(0, SOME_FATAL_ID);
    ASSERT_from_faultOut_SIZE(1);
    ASSERT_from_faultOut(0, FATAL_OCCURRED);
    // The call returns only once the fallback has run (the test fallback does not end the process)
    ASSERT_EQ(this->component.fallbackCount, 1);
    // The fault was reported before the thread was parked and the fallback invoked
    ASSERT_EQ(this->component.faultsReportedAtFallback, 1);
}

void FatalToFaultTester ::testRepeatedFatal() {
    this->invoke_to_FatalReceive(0, SOME_FATAL_ID);
    this->invoke_to_FatalReceive(0, SOME_FATAL_ID + 1);
    // FaultManager latches the fault; every FATAL is still forwarded so the report cannot be lost
    ASSERT_from_faultOut_SIZE(2);
    ASSERT_from_faultOut(1, FATAL_OCCURRED);
    ASSERT_EQ(this->component.fallbackCount, 2);
}

void FatalToFaultTester ::testUnconnectedFallback() {
    TestFatalToFault bare("bare");
    bare.init(TEST_INSTANCE_ID);
    bare.faultsReported = 0;
    bare.get_FatalReceive_InputPort(0)->invoke(SOME_FATAL_ID);
    ASSERT_EQ(bare.fallbackCount, 1);
    ASSERT_EQ(bare.faultsReportedAtFallback, 0);
}

// ----------------------------------------------------------------------
// Handlers
// ----------------------------------------------------------------------

void FatalToFaultTester ::from_faultOut_handler(FwIndexType portNum, const FaultConfig::Fault& id) {
    this->component.faultsReported++;
    this->pushFromPortEntry_faultOut(id);
}

}  // namespace FaultProtection

}  // namespace Svc
