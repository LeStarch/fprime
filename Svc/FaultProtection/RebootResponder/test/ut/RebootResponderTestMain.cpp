// ======================================================================
// \title  RebootResponderTestMain.cpp
// \author mstarch
// \brief  cpp file for RebootResponder component test main function
// ======================================================================

#include "Fw/Test/UnitTest.hpp"
#include "RebootResponderTester.hpp"

TEST(Nominal, DispatchReboots) {
    COMMENT("A dispatched REBOOT step is announced and the reboot hook is invoked");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testDispatchReboots();
}

TEST(OffNominal, HookReturnFails) {
    COMMENT("A reboot hook that declines to reboot fails the step");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testHookReturnFails();
}

TEST(OffNominal, CancelRefused) {
    COMMENT("Reboot cancellation is refused");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testCancelRefused();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
