// ======================================================================
// \title  SequenceResponderTestMain.cpp
// \author mstarch
// \brief  cpp file for SequenceResponder component test main function
// ======================================================================

#include "Fw/Test/UnitTest.hpp"
#include "SequenceResponderTester.hpp"

TEST(Nominal, Sequence) {
    COMMENT("A dispatched step runs <directory>/<step>.seq and completes with the sequence");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testNominalSequence();
}

TEST(Nominal, ConsecutiveSteps) {
    COMMENT("Consecutive steps run one after another");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testConsecutiveSteps();
}

TEST(OffNominal, SequenceFailure) {
    COMMENT("A failed sequence fails the step");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testSequenceFailure();
}

TEST(OffNominal, Busy) {
    COMMENT("A dispatch while a sequence is running is rejected");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testBusy();
}

TEST(OffNominal, Cancel) {
    COMMENT("Cancel stops the running sequence and suppresses its completion");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testCancel();
}

TEST(OffNominal, IdleInputs) {
    COMMENT("Cancel and completion with nothing running are harmless");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testIdleInputs();
}

TEST(OffNominal, FileNameTooLong) {
    COMMENT("A sequence file name that does not fit fails the step without truncation");
    REQUIREMENT("SVC_SEQUENCERESPONDER_007");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testFileNameTooLong();
}

TEST(OffNominal, UnconnectedSequencer) {
    COMMENT("A step dispatched with no sequencer connected fails");
    Svc::FaultProtection::SequenceResponderTester tester;
    tester.testUnconnectedSequencer();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
