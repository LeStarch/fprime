// ======================================================================
// \title  MonitoredCounterTester.hpp
// \author mstarch
// \brief  hpp file for MonitoredCounter component test harness implementation class
// ======================================================================

#ifndef Ref_MonitoredCounterTester_HPP
#define Ref_MonitoredCounterTester_HPP

#include "Ref/MonitoredCounter/MonitoredCounter.hpp"
#include "Ref/MonitoredCounter/MonitoredCounterGTestBase.hpp"

namespace Ref {

class MonitoredCounterTester final : public MonitoredCounterGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 100;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    // Default parameter values (see MonitoredCounter.fpp)
    static const U32 DEFAULT_COUNT_THRESHOLD = 10;
    static const U32 DEFAULT_LOCAL_ERROR_THRESHOLD = 2;
    static const U32 DEFAULT_SYSTEM_ERROR_THRESHOLD = 4;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object MonitoredCounterTester
    MonitoredCounterTester();

    //! Destroy object MonitoredCounterTester
    ~MonitoredCounterTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! Cycles below the threshold count without monitor errors
    void testCounting();

    //! Count above the threshold turns the monitor YELLOW then RED, reporting COUNTER_HIGH once
    void testFault();

    //! RESET_COUNT clears the count and the monitor recovers; a new excursion reports again
    void testResetRecovers();

    //! Disabled monitoring turns the monitor BLACK and suspends error counting
    void testMonitoringDisabled();

    //! Thresholds follow the parameters
    void testParameters();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

    //! Dispatch every message queued on the component (ports, commands, and state machine signals)
    void dispatchAll();

    //! Run `count` rate group cycles
    void cycle(U32 count);

    //! Cycle until the count reaches `count` (which must not be below the current count)
    void cycleTo(U32 count);

    //! Send RESET_COUNT and dispatch it
    void resetCount();

    //! Send SET_MONITORING and dispatch it
    void setMonitoring(const Fw::Enabled& enabled);

    //! Check the latest Count and ErrorCount telemetry
    void assertCounts(U32 count, U32 errors);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    MonitoredCounter component;

    //! Cycles run so far
    U32 m_cycles;
};

}  // namespace Ref

#endif
