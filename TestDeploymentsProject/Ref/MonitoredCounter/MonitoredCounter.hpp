// ======================================================================
// \title  MonitoredCounter.hpp
// \author mstarch
// \brief  hpp file for MonitoredCounter component implementation class
// ======================================================================

#ifndef Ref_MonitoredCounter_HPP
#define Ref_MonitoredCounter_HPP

#include "Ref/MonitoredCounter/MonitoredCounterComponentAc.hpp"

namespace Ref {

class MonitoredCounter final : public MonitoredCounterComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct MonitoredCounter object
    explicit MonitoredCounter(const char* const compName  //!< The component name
    );

    //! Destroy MonitoredCounter object
    ~MonitoredCounter();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for run
    //!
    //! Increments the count and cycles the monitor
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command RESET_COUNT
    void RESET_COUNT_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                U32 cmdSeq            //!< The command sequence number
                                ) override;

    //! Handler implementation for command SET_MONITORING
    void SET_MONITORING_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                   U32 cmdSeq,           //!< The command sequence number
                                   const Fw::Enabled& enabled) override;

    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action doBlack of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doBlack(SmId smId,
                                                           Svc_FaultProtection_MonitorMachine::Signal signal) override;

    //! Implementation for action doGreen of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doGreen(SmId smId,
                                                           Svc_FaultProtection_MonitorMachine::Signal signal) override;

    //! Implementation for action doYellow of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doYellow(SmId smId,
                                                            Svc_FaultProtection_MonitorMachine::Signal signal) override;

    //! Implementation for action doRed of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doRed(SmId smId,
                                                         Svc_FaultProtection_MonitorMachine::Signal signal) override;

    //! Implementation for action doErrorDecrease of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doErrorDecrease(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) override;

    //! Implementation for action doErrorIncrease of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doErrorIncrease(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) override;

    //! Implementation for action doSystemResponse of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doSystemResponse(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) override;

    //! Implementation for action doLocalResponse of state machine Svc_FaultProtection_MonitorMachine
    void Svc_FaultProtection_MonitorMachine_action_doLocalResponse(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) override;

    // ----------------------------------------------------------------------
    // Implementations for internal state machine guards
    // ----------------------------------------------------------------------

    //! Implementation for guard checkPrecondition of state machine Svc_FaultProtection_MonitorMachine
    bool Svc_FaultProtection_MonitorMachine_guard_checkPrecondition(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) const override;

    //! Implementation for guard performTest of state machine Svc_FaultProtection_MonitorMachine
    bool Svc_FaultProtection_MonitorMachine_guard_performTest(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) const override;

    //! Implementation for guard checkSystemThreshold of state machine Svc_FaultProtection_MonitorMachine
    bool Svc_FaultProtection_MonitorMachine_guard_checkSystemThreshold(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) const override;

    //! Implementation for guard checkLocalThreshold of state machine Svc_FaultProtection_MonitorMachine
    bool Svc_FaultProtection_MonitorMachine_guard_checkLocalThreshold(
        SmId smId,
        Svc_FaultProtection_MonitorMachine::Signal signal) const override;

    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Cache the threshold parameters once the parameter database has loaded them
    void parametersLoaded() override;

    //! Re-cache the threshold parameters when one is updated
    void parameterUpdated(FwPrmIdType id) override;

    //! Read the threshold parameters into the cache
    void cacheThresholds();

    //! Set the monitor color, emitting an event and telemetry on change
    void setColor(const MonitorColor& color);

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    U32 m_count;               //!< Cycle count
    U32 m_errors;              //!< Monitor error count
    MonitorColor m_color;      //!< Monitor color
    Fw::Enabled m_monitoring;  //!< Monitoring precondition
    bool m_fault_reported;     //!< COUNTER_HIGH has been reported for the current excursion
    U32 m_count_threshold;     //!< Cached COUNT_THRESHOLD parameter
    U32 m_local_threshold;     //!< Cached LOCAL_ERROR_THRESHOLD parameter
    U32 m_system_threshold;    //!< Cached SYSTEM_ERROR_THRESHOLD parameter
};

}  // namespace Ref

#endif
