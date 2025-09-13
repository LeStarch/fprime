// ----------------------------------------------------------------------
// TestMain.cpp
// ----------------------------------------------------------------------

#include "FaultProtectionTester.hpp"

TEST(Nominal, TestFatalResponse) {
    Svc::Tester tester;
    tester.test_fatal_response();
}

TEST(Nominal, TestNoResponse) {
    Svc::Tester tester;
    tester.test_no_response();
}

TEST(Nominal, TestSequenceResponse) {
    Svc::Tester tester;
    tester.test_sequence_response();
}

TEST(Nominal, TestClearToFatalResponse) {
    Svc::Tester tester;
    tester.test_clear_to_fatal_response();
}

TEST(Nominal, TestClearToNoResponse) {
    Svc::Tester tester;
    tester.test_clear_to_no_response();
}

TEST(Nominal, TestResetResponse) {
    Svc::Tester tester;
    tester.test_reset_response();
}

TEST(Nominal, TestSequentialResponse) {
    Svc::Tester tester;
    tester.test_serialized_response();
}

TEST(OffNominal, TestFatalOnBadResponse) {
    Svc::Tester tester;
    tester.test_fatal_on_bad_sequence();
}

TEST(OffNominal, TestFatalOnFailedResponse) {
    Svc::Tester tester;
    tester.test_fatal_on_failed_sequence();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
