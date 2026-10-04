// ======================================================================
// \title  RebootResponderTester.hpp
// \author mstarch
// \brief  hpp file for RebootResponder component test harness implementation class
// ======================================================================

#ifndef Svc_FaultProtection_RebootResponderTester_HPP
#define Svc_FaultProtection_RebootResponderTester_HPP

#include "Svc/FaultProtection/RebootResponder/RebootResponder.hpp"
#include "Svc/FaultProtection/RebootResponder/RebootResponderGTestBase.hpp"

namespace Svc {

namespace FaultProtection {

class RebootResponderTester final : public RebootResponderGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Reboot delay (run ticks) used by the tests
    static const FwSizeType REBOOT_DELAY_TICKS = 2;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object RebootResponderTester
    RebootResponderTester();

    //! Destroy object RebootResponderTester
    ~RebootResponderTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! A dispatched step announces the reboot and invokes the reboot hook once the delay has elapsed
    void testDispatchReboots();

    //! Ticks without a request never reboot
    void testTicksWithoutRequest();

    //! A second request while a reboot is pending does not restart the delay
    void testRepeatedRequest();

    //! A reboot hook that returns results in a failed step
    void testHookReturnFails();

    //! Cancel requests are refused
    void testCancelRefused();

  private:
    //! RebootResponder whose reboot hook records the request instead of ending the process
    class TestRebootResponder : public RebootResponder {
      public:
        explicit TestRebootResponder(const char* const compName) : RebootResponder(compName), rebootCount(0) {}
        U32 rebootCount;  //!< Number of reboot hook invocations

      protected:
        void doReboot() override { this->rebootCount++; }
    };

    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Tick the component `count` times
    void tick(FwSizeType count);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    TestRebootResponder component;
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
