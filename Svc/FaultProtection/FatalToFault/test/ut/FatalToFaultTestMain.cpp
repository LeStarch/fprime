// ======================================================================
// \title  FatalToFaultTestMain.cpp
// \author mstarch
// \brief  cpp file for FatalToFault component test main function
// ======================================================================

#include "FatalToFaultTester.hpp"
#include "Fw/Test/UnitTest.hpp"

TEST(Nominal, FatalReportsFault) {
    COMMENT("A FATAL event is reported as FATAL_OCCURRED");
    Svc::FaultProtection::FatalToFaultTester tester;
    tester.testFatalReportsFault();
}

TEST(Nominal, TicksWithoutFatal) {
    COMMENT("Ticks without a FATAL never invoke the fallback");
    Svc::FaultProtection::FatalToFaultTester tester;
    tester.testTicksWithoutFatal();
}

TEST(OffNominal, FallbackCountdown) {
    COMMENT("The fallback is invoked when the fault response has not ended the software in time");
    Svc::FaultProtection::FatalToFaultTester tester;
    tester.testFallbackCountdown();
}

TEST(OffNominal, RepeatedFatal) {
    COMMENT("Repeated FATALs are forwarded and do not restart the countdown");
    Svc::FaultProtection::FatalToFaultTester tester;
    tester.testRepeatedFatal();
}

TEST(OffNominal, UnconnectedFallback) {
    COMMENT("With no fault reporting connection the fallback is immediate");
    Svc::FaultProtection::FatalToFaultTester tester;
    tester.testUnconnectedFallback();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
