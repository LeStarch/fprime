// ======================================================================
// \title  FaultManagerTester.hpp
// \author mstarch
// \brief  hpp file for FaultManager component test harness implementation class
// ======================================================================

#ifndef Svc_FaultProtection_FaultManagerTester_HPP
#define Svc_FaultProtection_FaultManagerTester_HPP

#include "Svc/FaultProtection/FaultManager/FaultManager.hpp"
#include "Svc/FaultProtection/FaultManager/FaultManagerGTestBase.hpp"

namespace Svc {

namespace FaultProtection {

class FaultManagerTester final : public FaultManagerGTestBase {
  public:
    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 20;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    // Queue depth supplied to the component instance under test
    static const FwSizeType TEST_INSTANCE_QUEUE_DEPTH = 10;

    //! Default configuration: the number of ticks from an idle report until the first step is dispatched
    static const FwSizeType TICKS_TO_RESPONSE = FaultConfig::RESPONSE_COUNTDOWN_TICKS + 1;

  public:
    FaultManagerTester();
    ~FaultManagerTester();

  public:
    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    //! Ticks with no report leave the manager idle and silent
    void testIdle();

    //! A report is latched, counted down, responded to, and cleared on success
    void testNominalResponse();

    //! A duplicate report of a latched fault is ignored
    void testDuplicateReportIgnored();

    //! An invalid fault id is rejected
    void testInvalidReport();

    //! A disabled fault is reported but not responded to; re-enabling restores the response
    void testFaultDisabled();

    //! A failed step with failure mode FAULT fails the response and latches FAULT_RESPONSE_FAILURE
    void testStepFailureFault();

    //! A failed step with failure mode IGNORE is treated as success
    void testStepFailureIgnore();

    //! A failed step with failure mode DEFER completes the response then fails it
    void testStepFailureDefer();

    //! A disabled response skips its steps and clears the latch
    void testResponseDisabled();

    //! A higher-precedence report preempts the active response; the preempted fault is responded to afterwards
    void testPreemption();

    //! A lower-precedence report during a response waits for the active response to complete
    void testNoPreemptionLowerPrecedence();

    //! When two faults are latched during the countdown the higher precedence response is selected first
    void testPrecedenceSelection();

    //! Completions not matching the active step are rejected
    void testUnexpectedCompletion();

    //! Commands reject enumeration values outside the configured ranges
    void testCommandValidation();

    //! A failed response to FAULT_RESPONSE_FAILURE does not escalate (no recursion)
    void testResponseFailureNoRecursion();
    void testMultiStepResponse();
    void testDeferThenContinue();
    void testStepTimeout();
    void testStepPortUnconnected();
    void testIgnoredReportThrottle();
    void testDisableClearsLatch();
    void testTableParameters();

  private:
    // ----------------------------------------------------------------------
    // Handlers for typed from ports
    // ----------------------------------------------------------------------

    void from_stepDispatchOut_handler(FwIndexType portNum,
                                      const FaultConfig::Response& response,
                                      const FaultConfig::Step& step,
                                      const FaultConfig::Context& context) override;

    void from_stepCancelOut_handler(FwIndexType portNum) override;

    //! Print text events to aid debugging
    void textLogIn(FwEventIdType id,
                   const Fw::Time& timeTag,
                   const Fw::LogSeverity severity,
                   const Fw::TextLogString& text) override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Dispatch every message queued on the component (ports, commands, and state machine signals)
    void dispatchAll(FaultManager& target);

    //! Report a fault and dispatch the internal report handling
    void report(const FaultConfig::Fault& fault);

    //! Tick the manager `count` times
    void tick(FwSizeType count = 1);

    //! Complete the active step with the given status and dispatch
    void complete(const Fw::Success& status, const FaultConfig::Response& response, const FaultConfig::Step& step);

    //! Assert a single dispatch on the given port of the given response/step, then clear the history
    void assertDispatched(const FaultConfig::Port& port,
                          const FaultConfig::Response& response,
                          const FaultConfig::Step& step);

    //! Assert nothing has been dispatched
    void assertNotDispatched();

    //! Drive a report through countdown to its first dispatched step
    void reportAndDispatch(const FaultConfig::Fault& fault,
                           const FaultConfig::Port& port,
                           const FaultConfig::Response& response,
                           const FaultConfig::Step& step);

    //! Remap a fault's response via the FAULT_RESPONSE_TABLE parameter
    void remapFault(const FaultConfig::Fault& fault, const FaultConfig::Response& response, U8 precedence);

    //! Send a command and dispatch it, asserting the given response
    void sendCommandSetFaultEnabled(const FaultConfig::Fault& fault,
                                    const Fw::Enabled& enabled,
                                    const Fw::CmdResponse& expected);
    void sendCommandSetResponseEnabled(const FaultConfig::Response& response,
                                       const Fw::Enabled& enabled,
                                       const Fw::CmdResponse& expected);
    void sendCommandUpdateStepFailureMode(const FaultConfig::Step& step,
                                          const FaultConfig::FailureMode& mode,
                                          const Fw::CmdResponse& expected);

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  private:
    //! The component under test
    FaultManager component;

    //! Port number of the most recent step dispatch
    FwIndexType m_last_dispatch_port;
    FwIndexType m_last_cancel_port;
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
