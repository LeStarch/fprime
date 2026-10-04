// ======================================================================
// \title  MonitoredCounterTestMain.cpp
// \author mstarch
// \brief  cpp file for MonitoredCounter component test main function
// ======================================================================

#include "Fw/Test/UnitTest.hpp"
#include "MonitoredCounterTester.hpp"

TEST(Nominal, Counting) {
    COMMENT("Cycles below the threshold count without monitor errors");
    Ref::MonitoredCounterTester tester;
    tester.testCounting();
}

TEST(Nominal, Fault) {
    COMMENT("Count above the threshold turns the monitor YELLOW then RED, reporting COUNTER_HIGH once");
    Ref::MonitoredCounterTester tester;
    tester.testFault();
}

TEST(Nominal, ResetRecovers) {
    COMMENT("RESET_COUNT clears the count, the monitor recovers, and a new excursion reports again");
    Ref::MonitoredCounterTester tester;
    tester.testResetRecovers();
}

TEST(OffNominal, DefaultDisabled) {
    COMMENT("Monitoring starts disabled so a deployment without response sequences does not fault on its own");
    Ref::MonitoredCounterTester tester;
    tester.testDefaultDisabled();
}

TEST(OffNominal, MonitoringDisabled) {
    COMMENT("Disabled monitoring turns the monitor BLACK and suspends error counting");
    Ref::MonitoredCounterTester tester;
    tester.testMonitoringDisabled();
}

TEST(Nominal, Parameters) {
    COMMENT("Thresholds follow the parameters");
    Ref::MonitoredCounterTester tester;
    tester.testParameters();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
