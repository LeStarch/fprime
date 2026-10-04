// ======================================================================
// \title  FatalToFaultTester.hpp
// \author mstarch
// \brief  hpp file for FatalToFault component test harness implementation class
// ======================================================================

#ifndef Svc_FaultProtection_FatalToFaultTester_HPP
#define Svc_FaultProtection_FatalToFaultTester_HPP

#include "Svc/FaultProtection/FatalToFault/FatalToFault.hpp"
#include "Svc/FaultProtection/FatalToFault/FatalToFaultGTestBase.hpp"

namespace Svc {

namespace FaultProtection {

class FatalToFaultTester final : public FatalToFaultGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Fallback countdown used by the tests
    static const FwSizeType FALLBACK_TICKS = 3;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object FatalToFaultTester
    FatalToFaultTester();

    //! Destroy object FatalToFaultTester
    ~FatalToFaultTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! A FATAL is reported as the FATAL_OCCURRED fault
    void testFatalReportsFault();

    //! Ticks without a FATAL never invoke the fallback
    void testTicksWithoutFatal();

    //! The fallback is invoked once the countdown after a FATAL expires
    void testFallbackCountdown();

    //! Repeated FATALs report once and do not restart the countdown
    void testRepeatedFatal();

    //! With no fault reporting connection the fallback is immediate
    void testUnconnectedFallback();

  private:
    //! FatalToFault whose fallback records the invocation instead of aborting the process
    class TestFatalToFault : public FatalToFault {
      public:
        explicit TestFatalToFault(const char* const compName) : FatalToFault(compName), fallbackCount(0) {}
        U32 fallbackCount;  //!< Number of fallback invocations

      protected:
        void fallback() override { this->fallbackCount++; }
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
    TestFatalToFault component;
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
