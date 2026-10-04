// ======================================================================
// \title  FaultManagerTestMain.cpp
// \author mstarch
// \brief  cpp file for FaultManager component test main function
// ======================================================================

#include "FaultManagerTester.hpp"
#include "Fw/Test/UnitTest.hpp"

using Svc::FaultProtection::FaultManagerTester;

TEST(Nominal, Idle) {
    COMMENT("Ticks without a fault report leave the FaultManager idle");
    FaultManagerTester tester;
    tester.testIdle();
}

TEST(Nominal, Response) {
    COMMENT("A report is latched, counted down, dispatched, completed, and the latch cleared");
    REQUIREMENT("SVC-FAULTMANAGER-001");
    FaultManagerTester tester;
    tester.testNominalResponse();
}

TEST(Nominal, PrecedenceSelection) {
    COMMENT("Multiple latched faults are responded to in precedence order");
    FaultManagerTester tester;
    tester.testPrecedenceSelection();
}

TEST(Nominal, Preemption) {
    COMMENT("A higher-precedence report cancels the active step and the preempted fault is responded to afterwards");
    FaultManagerTester tester;
    tester.testPreemption();
}

TEST(Nominal, NoPreemptionLowerPrecedence) {
    COMMENT("A lower-precedence report waits for the active response");
    FaultManagerTester tester;
    tester.testNoPreemptionLowerPrecedence();
}

TEST(Nominal, ResponseDisabled) {
    COMMENT("Disabled responses skip their steps");
    FaultManagerTester tester;
    tester.testResponseDisabled();
}

TEST(Nominal, MultiStepResponse) {
    COMMENT("A multi-step response dispatches each step in order to its configured port");
    FaultManagerTester tester;
    tester.testMultiStepResponse();
}

TEST(OffNominal, DeferThenContinue) {
    COMMENT("Failure mode DEFER runs the remaining steps before failing the response");
    FaultManagerTester tester;
    tester.testDeferThenContinue();
}

TEST(OffNominal, StepTimeout) {
    COMMENT("A step without completion within timeoutTicks is canceled and failed");
    FaultManagerTester tester;
    tester.testStepTimeout();
}

TEST(OffNominal, StepPortUnconnected) {
    COMMENT("A step whose dispatch port is unconnected fails without asserting");
    FaultManagerTester tester;
    tester.testStepPortUnconnected();
}

TEST(OffNominal, IgnoredReportThrottle) {
    COMMENT("FaultIgnored is throttled and the throttle clears every run tick");
    FaultManagerTester tester;
    tester.testIgnoredReportThrottle();
}

TEST(OffNominal, DisableClearsLatch) {
    COMMENT("Disabling a latched fault discards its pending report");
    FaultManagerTester tester;
    tester.testDisableClearsLatch();
}

TEST(Nominal, TableParameters) {
    COMMENT("RESPONSE_TABLE and STEP_TABLE parameters override the defaults when valid");
    REQUIREMENT("SVC-FAULTMANAGER-003");
    FaultManagerTester tester;
    tester.testTableParameters();
}

TEST(OffNominal, DuplicateReportIgnored) {
    COMMENT("A report of an already latched fault is ignored");
    FaultManagerTester tester;
    tester.testDuplicateReportIgnored();
}

TEST(OffNominal, InvalidReport) {
    COMMENT("An out-of-range fault id is rejected without asserting");
    FaultManagerTester tester;
    tester.testInvalidReport();
}

TEST(OffNominal, FaultDisabled) {
    COMMENT("A disabled fault is not responded to until re-enabled");
    FaultManagerTester tester;
    tester.testFaultDisabled();
}

TEST(OffNominal, StepFailureFault) {
    COMMENT("Failure mode FAULT fails the response and reports FAULT_RESPONSE_FAILURE");
    FaultManagerTester tester;
    tester.testStepFailureFault();
}

TEST(OffNominal, StepFailureIgnore) {
    COMMENT("Failure mode IGNORE continues the response");
    FaultManagerTester tester;
    tester.testStepFailureIgnore();
}

TEST(OffNominal, StepFailureDefer) {
    COMMENT("Failure mode DEFER finishes the response then reports FAULT_RESPONSE_FAILURE");
    FaultManagerTester tester;
    tester.testStepFailureDefer();
}

TEST(OffNominal, UnexpectedCompletion) {
    COMMENT("Completions not matching the active step are rejected");
    FaultManagerTester tester;
    tester.testUnexpectedCompletion();
}

TEST(OffNominal, CommandValidation) {
    COMMENT("Commands with out-of-range enumerations respond VALIDATION_ERROR");
    FaultManagerTester tester;
    tester.testCommandValidation();
}

TEST(OffNominal, ResponseFailureNoRecursion) {
    COMMENT("A failed response to FAULT_RESPONSE_FAILURE does not re-report FAULT_RESPONSE_FAILURE");
    FaultManagerTester tester;
    tester.testResponseFailureNoRecursion();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
