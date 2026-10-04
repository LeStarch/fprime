// ======================================================================
// \title  RebootResponderTestMain.cpp
// \author mstarch
// \brief  cpp file for RebootResponder component test main function
// ======================================================================

#include "Fw/Test/UnitTest.hpp"
#include "RebootResponderTester.hpp"

TEST(Nominal, DispatchReboots) {
    COMMENT("A dispatched REBOOT step is announced and the reboot hook is invoked after the delay");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testDispatchReboots();
}

TEST(Nominal, TicksWithoutRequest) {
    COMMENT("Ticks without a reboot request never reboot");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testTicksWithoutRequest();
}

TEST(OffNominal, RepeatedRequest) {
    COMMENT("A second request while a reboot is pending does not restart the delay");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testRepeatedRequest();
}

TEST(OffNominal, HookReturnFails) {
    COMMENT("A reboot hook that declines to reboot fails the step");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testHookReturnFails();
}

TEST(OffNominal, CancelRefused) {
    COMMENT("Reboot cancellation is refused and the reboot proceeds");
    Svc::FaultProtection::RebootResponderTester tester;
    tester.testCancelRefused();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
