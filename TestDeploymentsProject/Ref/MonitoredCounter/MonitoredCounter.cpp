// ======================================================================
// \title  MonitoredCounter.cpp
// \author mstarch
// \brief  cpp file for MonitoredCounter component implementation class
// ======================================================================

#include "Ref/MonitoredCounter/MonitoredCounter.hpp"

namespace Ref {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

MonitoredCounter ::MonitoredCounter(const char* const compName)
    : MonitoredCounterComponentBase(compName),
      m_count(0),
      m_errors(0),
      m_color(MonitorColor::GREEN),
      m_monitoring(Fw::Enabled::DISABLED),
      m_fault_reported(false),
      m_count_threshold(0),
      m_local_threshold(0),
      m_system_threshold(0) {
    this->cacheThresholds();
}

MonitoredCounter ::~MonitoredCounter() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void MonitoredCounter ::run_handler(FwIndexType portNum, U32 context) {
    this->m_count++;
    this->tlmWrite_Count(this->m_count);
    // The signal is queued: the monitor evaluates this cycle once the run handler returns
    this->monitorMachine_sendSignal_cycle();
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void MonitoredCounter ::RESET_COUNT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->log_ACTIVITY_HI_CountReset(this->m_count);
    this->m_count = 0;
    this->tlmWrite_Count(this->m_count);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void MonitoredCounter ::SET_MONITORING_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::Enabled& enabled) {
    this->m_monitoring = enabled;
    this->log_ACTIVITY_HI_MonitoringSet(enabled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doBlack(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    this->setColor(MonitorColor::BLACK);
    this->tlmWrite_ErrorCount(this->m_errors);
}

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doGreen(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    this->m_fault_reported = false;
    this->setColor(MonitorColor::GREEN);
}

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doYellow(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    this->setColor(MonitorColor::YELLOW);
}

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doRed(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    this->setColor(MonitorColor::RED);
}

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doErrorDecrease(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    this->m_errors = (this->m_errors > 0) ? (this->m_errors - 1) : 0;
    this->tlmWrite_ErrorCount(this->m_errors);
}

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doErrorIncrease(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    if (this->m_errors < std::numeric_limits<U32>::max()) {
        this->m_errors++;
    }
    this->tlmWrite_ErrorCount(this->m_errors);
}

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doSystemResponse(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    // Report once per excursion: FaultManager latches the report, so repeats would only be ignored
    if (not this->m_fault_reported) {
        this->m_fault_reported = true;
        this->log_WARNING_HI_CountHighFault(this->m_count, this->m_errors);
        if (this->isConnected_faultOut_OutputPort(0)) {
            this->faultOut_out(0, FaultConfig::Fault::COUNTER_HIGH);
        }
    }
}

void MonitoredCounter ::Svc_FaultProtection_MonitorMachine_action_doLocalResponse(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) {
    this->log_WARNING_LO_CountHighWarning(this->m_count, this->m_errors);
}

// ----------------------------------------------------------------------
// Implementations for internal state machine guards
// ----------------------------------------------------------------------

bool MonitoredCounter ::Svc_FaultProtection_MonitorMachine_guard_checkPrecondition(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) const {
    return this->m_monitoring == Fw::Enabled::ENABLED;
}

bool MonitoredCounter ::Svc_FaultProtection_MonitorMachine_guard_performTest(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) const {
    return this->m_count > this->m_count_threshold;
}

bool MonitoredCounter ::Svc_FaultProtection_MonitorMachine_guard_checkSystemThreshold(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) const {
    return this->m_errors >= this->m_system_threshold;
}

bool MonitoredCounter ::Svc_FaultProtection_MonitorMachine_guard_checkLocalThreshold(
    SmId smId,
    Svc_FaultProtection_MonitorMachine::Signal signal) const {
    return this->m_errors >= this->m_local_threshold;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void MonitoredCounter ::setColor(const MonitorColor& color) {
    if (color != this->m_color) {
        this->log_ACTIVITY_LO_MonitorColorChanged(this->m_color, color, this->m_count, this->m_errors);
        this->m_color = color;
        this->tlmWrite_Monitor(this->m_color);
    }
}

void MonitoredCounter ::parametersLoaded() {
    this->cacheThresholds();
}

void MonitoredCounter ::parameterUpdated(FwPrmIdType id) {
    this->cacheThresholds();
}

void MonitoredCounter ::cacheThresholds() {
    // Parameters have defaults, hence the getters return the default when the parameter database has no value
    Fw::ParamValid valid = Fw::ParamValid::INVALID;
    this->m_count_threshold = this->paramGet_COUNT_THRESHOLD(valid);
    this->m_local_threshold = this->paramGet_LOCAL_ERROR_THRESHOLD(valid);
    this->m_system_threshold = this->paramGet_SYSTEM_ERROR_THRESHOLD(valid);
}

}  // namespace Ref
