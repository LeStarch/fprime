// ======================================================================
// \title  SequenceResponderTester.hpp
// \author mstarch
// \brief  hpp file for SequenceResponder component test harness implementation class
// ======================================================================

#ifndef Svc_FaultProtection_SequenceResponderTester_HPP
#define Svc_FaultProtection_SequenceResponderTester_HPP

#include "Svc/FaultProtection/SequenceResponder/SequenceResponder.hpp"
#include "Svc/FaultProtection/SequenceResponder/SequenceResponderGTestBase.hpp"

namespace Svc {

namespace FaultProtection {

class SequenceResponderTester final : public SequenceResponderGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object SequenceResponderTester
    SequenceResponderTester();

    //! Destroy object SequenceResponderTester
    ~SequenceResponderTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! A dispatched step runs `<directory>/<step>.seq` and completes with the sequence
    void testNominalSequence();

    //! Consecutive steps run one after another
    void testConsecutiveSteps();

    //! A failed sequence fails the step
    void testSequenceFailure();

    //! A dispatch while a sequence is running is rejected
    void testBusy();

    //! Cancel stops the running sequence and suppresses its completion
    void testCancel();

    //! Cancel and sequence completion with nothing running are harmless
    void testIdleInputs();

    //! A step dispatched with no sequencer connected fails
    void testUnconnectedSequencer();
    void testFileNameTooLong();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Dispatch a step and check the sequence run request
    void dispatch(const FaultConfig::Response& response, const FaultConfig::Step& step, const char* expectedFile);

    //! Report sequence completion
    void sequenceDone(const Fw::CmdResponse& status);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    SequenceResponder component;
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
