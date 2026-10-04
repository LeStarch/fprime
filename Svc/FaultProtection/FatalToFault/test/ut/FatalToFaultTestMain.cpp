// ======================================================================
// \title  FatalToFaultTestMain.cpp
// \author mstarch
// \brief  cpp file for FatalToFault component test main function
// ======================================================================

#include "FatalToFaultTester.hpp"
#include "Fw/Test/UnitTest.hpp"

TEST(Nominal, FatalReportsFaultThenFallsBack) {
    COMMENT("A FATAL is reported as FATAL_OCCURRED; the asserting thread is parked, then the fallback is invoked");
    Svc::FaultProtection::FatalToFaultTester tester;
    tester.testFatalReportsFaultThenFallsBack();
}

TEST(OffNominal, RepeatedFatal) {
    COMMENT("Repeated FATALs are each forwarded and each invoke the fallback");
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
