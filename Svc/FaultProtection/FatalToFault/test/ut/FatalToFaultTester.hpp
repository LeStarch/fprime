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

    // Fallback delay used by the tests (microseconds): short, the thread is parked for real
    static const U32 FALLBACK_DELAY_USECONDS = 10000;

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

    //! A FATAL is reported as the FATAL_OCCURRED fault, the thread is parked, then the fallback is invoked
    void testFatalReportsFaultThenFallsBack();

    //! Every FATAL is forwarded (the fault system latches); each invokes the fallback after its delay
    void testRepeatedFatal();

    //! With no fault reporting connection the fallback is immediate
    void testUnconnectedFallback();

  private:
    //! FatalToFault whose fallback records the invocation instead of aborting the process
    class TestFatalToFault : public FatalToFault {
      public:
        explicit TestFatalToFault(const char* const compName)
            : FatalToFault(compName), fallbackCount(0), faultsReportedAtFallback(0) {}
        U32 fallbackCount;             //!< Number of fallback invocations
        U32 faultsReportedAtFallback;  //!< Value of the recorder at the time of the last fallback
        U32 faultsReported;            //!< Fault reports observed so far (set by the tester)

      protected:
        void fallback() override {
            this->fallbackCount++;
            this->faultsReportedAtFallback = this->faultsReported;
        }
    };

    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Handler for faultOut: records the report count on the component such that ordering can be checked
    void from_faultOut_handler(FwIndexType portNum, const FaultConfig::Fault& id) override;

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
