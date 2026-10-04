// ======================================================================
// \title  FaultManager.cpp
// \author mstarch
// \brief  cpp file for FaultManager component implementation class
// ======================================================================

#include "Svc/FaultProtection/FaultManager/FaultManager.hpp"

#include "Fw/Logger/Logger.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

FaultManager ::FaultManager(const char* const compName)
    : FaultManagerComponentBase(compName),
      m_fault_parameter(),
      m_response_parameter(),
      m_step_parameter(),
      m_response_definition_table(),
      m_step_definition_table(),
      m_sm_state(),
      m_faults_reported(0),
      m_faults_ignored(0),
      m_responses_completed(0),
      m_responses_failed(0) {
    // Step failure modes default to those of the step definition table
    for (FwSizeType i = 0; i < StepDefinitionTable::SIZE; i++) {
        const StepDefinitionEntry& entry = this->m_step_definition_table[i];
        if (FaultManager::isConfiguredStep(entry.get_step())) {
            this->m_step_parameter[entry.get_step()] = entry.get_failureMode();
        }
    }
    this->m_sm_state.countdown = 0;
    this->m_sm_state.response_result = Fw::Success::SUCCESS;
    this->m_sm_state.active_fault_index = NO_ACTIVE_INDEX;
    this->m_sm_state.active_response_index = NO_ACTIVE_INDEX;
    this->m_sm_state.active_step_index = 0;
    this->m_sm_state.active_step = FaultConfig::Step::SKIP;
    for (FwSizeType i = 0; i < FaultConfig::Fault::NUM_FAULTS; i++) {
        this->m_sm_state.latched_fault_reports[i] = false;
    }
    this->registerExternalParameters(this);
}

FaultManager ::~FaultManager() {}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void FaultManager ::reportIn_handler(FwIndexType portNum, const FaultConfig::Fault& id) {
    // Fault reports may originate from any component: never assert on their contents
    if (not FaultManager::isConfiguredFault(id)) {
        this->log_WARNING_HI_FaultInvalid(static_cast<U8>(id.e));
        return;
    }
    // Latch the report synchronously such that it cannot be lost to a full queue. All other processing (events,
    // telemetry, preemption) happens on the component's thread.
    bool expected = false;
    const bool newly_latched = this->m_sm_state.latched_fault_reports[id.e].compare_exchange_strong(expected, true);
    this->handleReport_internalInterfaceInvoke(id, newly_latched);
}

void FaultManager ::run_handler(FwIndexType portNum, U32 context) {
    // Ignored-report events are throttled per tick: a flapping reporter is bounded without being silenced forever
    this->log_WARNING_LO_FaultIgnored_ThrottleClear();
    this->log_WARNING_LO_FaultDisabled_ThrottleClear();
    this->log_WARNING_HI_FaultInvalid_ThrottleClear();
    this->faultManagerStateMachine_sendSignal_Tick();
}

void FaultManager ::stepCompletionIn_handler(FwIndexType portNum,
                                             const Fw::Success& status,
                                             const FaultConfig::Response& response,
                                             const FaultConfig::Step& step) {
    const bool in_step = (this->faultManagerStateMachine_getState() ==
                          Svc_FaultProtection_FaultManagerStateMachine::State::RESPONSE_DISPATCH_STEP);
    const bool matches_response =
        (this->m_sm_state.active_response_index != NO_ACTIVE_INDEX) &&
        (this->m_response_definition_table[this->m_sm_state.active_response_index].get_response() == response);
    const bool matches_step =
        (this->m_sm_state.active_step != FaultConfig::Step::SKIP) && (this->m_sm_state.active_step == step);
    if (in_step && matches_response && matches_step) {
        this->handleStepResult(status);
    } else {
        this->log_WARNING_HI_UnexpectedStepCompleted(step, response);
    }
}

// ----------------------------------------------------------------------
// Handler implementations for internal ports
// ----------------------------------------------------------------------

void FaultManager ::handleReport_internalInterfaceHandler(const FaultConfig::Fault& fault, bool latched) {
    if (not latched) {
        this->log_WARNING_LO_FaultIgnored(fault);
        this->m_faults_ignored++;
        this->writeTelemetry();
        return;
    }
    const FwSizeType fault_index = this->faultToFaultEntryIndex(fault);
    const bool enabled = (fault_index != NO_ACTIVE_INDEX) &&
                         (this->m_fault_parameter[fault_index].get_enabled() == Fw::Enabled::ENABLED);
    if (not enabled) {
        this->m_sm_state.latched_fault_reports[fault.e] = false;
        this->log_WARNING_LO_FaultDisabled(fault);
        this->m_faults_ignored++;
        this->writeTelemetry();
        return;
    }
    this->log_ACTIVITY_HI_FaultReported(fault);
    this->m_faults_reported++;
    this->writeTelemetry();

    // Preempt the active response when this report outranks the fault being responded to
    const bool responding = (this->faultManagerStateMachine_getState() ==
                             Svc_FaultProtection_FaultManagerStateMachine::State::RESPONSE_DISPATCH_STEP) &&
                            (this->m_sm_state.active_fault_index != NO_ACTIVE_INDEX);
    if (responding) {
        const U8 active_precedence = this->m_fault_parameter[this->m_sm_state.active_fault_index].get_precedence();
        if (this->m_fault_parameter[fault_index].get_precedence() > active_precedence) {
            this->m_sm_state.preempted_by = fault;
            this->faultManagerStateMachine_sendSignal_Preempt();
        }
    }
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

void FaultManager ::SET_FAULT_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                 U32 cmdSeq,
                                                 const FaultConfig::Fault& fault,
                                                 const Fw::Enabled& enabled) {
    const FwSizeType index = this->faultToFaultEntryIndex(fault);
    if (index == NO_ACTIVE_INDEX) {
        this->log_WARNING_LO_InvalidCommandArgument(static_cast<U8>(fault.e));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    this->m_fault_parameter[index].set_enabled(enabled);
    // A disabled fault must not respond when re-enabled on the strength of a stale report
    if (enabled == Fw::Enabled::DISABLED) {
        this->m_sm_state.latched_fault_reports[fault.e] = false;
    }
    this->log_ACTIVITY_HI_FaultEnabledSet(fault, enabled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::SET_RESPONSE_ENABLED_cmdHandler(FwOpcodeType opCode,
                                                    U32 cmdSeq,
                                                    const FaultConfig::Response& response,
                                                    const Fw::Enabled& enabled) {
    if (not FaultManager::isConfiguredResponse(response)) {
        this->log_WARNING_LO_InvalidCommandArgument(static_cast<U8>(response.e));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    this->m_response_parameter[response.e] = enabled;
    this->log_ACTIVITY_HI_ResponseEnabledSet(response, enabled);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void FaultManager ::UPDATE_STEP_FAILURE_MODE_cmdHandler(FwOpcodeType opCode,
                                                        U32 cmdSeq,
                                                        const FaultConfig::Step& step,
                                                        const FaultConfig::FailureMode& failureMode) {
    if (not FaultManager::isConfiguredStep(step)) {
        this->log_WARNING_LO_InvalidCommandArgument(static_cast<U8>(step.e));
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
    }
    this->m_step_parameter[step.e] = failureMode;
    this->log_ACTIVITY_HI_StepFailureModeSet(step, failureMode);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

// ----------------------------------------------------------------------
// Implementations for external parameter delegate serialization
// ----------------------------------------------------------------------

Fw::SerializeStatus FaultManager ::deserializeParam(const FwPrmIdType base_id,
                                                    const FwPrmIdType local_id,
                                                    const Fw::ParamValid prmStat,
                                                    Fw::SerialBufferBase& buff) {
    // Deserialize into a scratch copy such that a malformed parameter leaves the active table untouched
    Fw::SerializeStatus status = Fw::SerializeStatus::FW_DESERIALIZE_FORMAT_ERROR;
    switch (local_id) {
        case PARAMID_FAULT_RESPONSE_TABLE: {
            FaultResponseTable table;
            status = table.deserializeFrom(buff);
            if ((status == Fw::FW_SERIALIZE_OK) && (not FaultManager::isValidFaultTable(table))) {
                status = Fw::SerializeStatus::FW_DESERIALIZE_FORMAT_ERROR;
            }
            if (status == Fw::FW_SERIALIZE_OK) {
                this->m_fault_parameter = table;
            }
            break;
        }
        case PARAMID_RESPONSE_TABLE: {
            ResponsesEnabled table;
            status = table.deserializeFrom(buff);
            if (status == Fw::FW_SERIALIZE_OK) {
                this->m_response_parameter = table;
            }
            break;
        }
        case PARAMID_STEP_TABLE: {
            StepFailureModes table;
            status = table.deserializeFrom(buff);
            // The parameter has no model default: when nothing is stored, the step definition table (loaded at
            // construction) remains in force
            if ((status == Fw::FW_SERIALIZE_OK) && (prmStat == Fw::ParamValid::VALID)) {
                this->m_step_parameter = table;
            }
            break;
        }
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(local_id));
            break;
    }
    return status;
}

Fw::SerializeStatus FaultManager ::serializeParam(const FwPrmIdType base_id,
                                                  const FwPrmIdType local_id,
                                                  Fw::SerialBufferBase& buff) const {
    Fw::SerializeStatus status = Fw::SerializeStatus::FW_SERIALIZE_FORMAT_ERROR;
    switch (local_id) {
        case PARAMID_FAULT_RESPONSE_TABLE:
            status = this->m_fault_parameter.serializeTo(buff);
            break;
        case PARAMID_RESPONSE_TABLE:
            status = this->m_response_parameter.serializeTo(buff);
            break;
        case PARAMID_STEP_TABLE:
            status = this->m_step_parameter.serializeTo(buff);
            break;
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(local_id));
            break;
    }
    return status;
}

// ----------------------------------------------------------------------
// Implementations for internal state machine actions
// ----------------------------------------------------------------------

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_startCountdown(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    this->m_sm_state.countdown = FaultConfig::RESPONSE_COUNTDOWN_TICKS;
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_decrementCountdown(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    this->m_sm_state.countdown = (this->m_sm_state.countdown > 0) ? this->m_sm_state.countdown - 1 : 0;
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_selectResponse(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    this->m_sm_state.response_result = Fw::Success::SUCCESS;
    this->m_sm_state.active_fault_index = NO_ACTIVE_INDEX;
    this->m_sm_state.active_response_index = NO_ACTIVE_INDEX;
    this->m_sm_state.active_step_index = 0;
    this->m_sm_state.active_step = FaultConfig::Step::SKIP;

    // Select the highest-precedence enabled, latched fault. Ties resolve to the earliest table entry.
    bool found = false;
    U8 current_precedence = 0;
    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        const FaultResponseEntry& entry = this->m_fault_parameter[i];
        const FaultConfig::Fault fault = entry.get_fault();
        if (not FaultManager::isConfiguredFault(fault)) {
            continue;
        }
        const bool latched = this->m_sm_state.latched_fault_reports[fault.e];
        const bool enabled = (entry.get_enabled() == Fw::Enabled::ENABLED);
        if (latched && enabled && ((not found) || (entry.get_precedence() > current_precedence))) {
            current_precedence = entry.get_precedence();
            this->m_sm_state.active_fault_index = i;
            found = true;
        }
    }
    if (found) {
        const FaultResponseEntry& entry = this->m_fault_parameter[this->m_sm_state.active_fault_index];
        this->m_sm_state.active_response_index = this->responseToResponseEntryIndex(entry.get_response());
        if (this->m_sm_state.active_response_index == NO_ACTIVE_INDEX) {
            // Configuration error: a fault maps to an undefined response. Drop the report rather than loop forever.
            Fw::Logger::log("[CRITICAL] FaultManager: fault %d maps to undefined response %d; report discarded\n",
                            entry.get_fault(), entry.get_response());
            this->m_sm_state.latched_fault_reports[entry.get_fault()] = false;
            this->m_sm_state.active_fault_index = NO_ACTIVE_INDEX;
        } else {
            this->log_ACTIVITY_HI_ResponseStarted(entry.get_response(), entry.get_fault());
        }
    }
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_completeResponse(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    if ((this->m_sm_state.active_response_index != NO_ACTIVE_INDEX) &&
        (this->m_sm_state.active_fault_index != NO_ACTIVE_INDEX)) {
        const FaultConfig::Fault fault = this->m_fault_parameter[this->m_sm_state.active_fault_index].get_fault();
        const FaultConfig::Response response =
            this->m_response_definition_table[this->m_sm_state.active_response_index].get_response();
        if (signal == Svc_FaultProtection_FaultManagerStateMachine::Signal::Preempt) {
            // Leave the latch set: the preempted fault is responded to once the higher-precedence response completes
            this->cancelActiveStep();
            this->log_WARNING_LO_ResponsePreempted(response, fault, this->m_sm_state.preempted_by);
        } else if (this->m_sm_state.response_result == Fw::Success::SUCCESS) {
            this->clearLatchesForResponse(response);
            this->m_responses_completed++;
            this->log_ACTIVITY_HI_ResponseCompleted(response, fault);
        } else {
            // Clear the triggering fault to prevent re-running the failed response, then escalate
            this->m_sm_state.latched_fault_reports[fault.e] = false;
            this->m_responses_failed++;
            this->log_WARNING_HI_ResponseFailed(response, fault);
            if (fault == FaultConfig::Fault::FAULT_RESPONSE_FAILURE) {
                Fw::Logger::log("[CRITICAL] FaultManager: response to FAULT_RESPONSE_FAILURE failed; not escalating\n");
            } else {
                this->reportInternalFault(FaultConfig::Fault::FAULT_RESPONSE_FAILURE);
            }
        }
        this->writeTelemetry();
    }
    this->m_sm_state.active_fault_index = NO_ACTIVE_INDEX;
    this->m_sm_state.active_response_index = NO_ACTIVE_INDEX;
    this->m_sm_state.active_step_index = 0;
    this->m_sm_state.active_step = FaultConfig::Step::SKIP;
    this->m_sm_state.response_result = Fw::Success::SUCCESS;
}

void FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_action_dispatchStep(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) {
    // Nothing to dispatch: an empty or unselected response finishes immediately
    if ((this->m_sm_state.active_response_index == NO_ACTIVE_INDEX) ||
        (this->m_sm_state.active_step_index >= FaultConfig::FAULT_RESPONSE_STEP_COUNT)) {
        this->faultManagerStateMachine_sendSignal_StepSuccessful();
        return;
    }
    const ResponseDefinitionEntry& response_entry =
        this->m_response_definition_table[this->m_sm_state.active_response_index];
    const FaultConfig::Response response = response_entry.get_response();
    const FaultConfig::Step step = response_entry.get_steps()[this->m_sm_state.active_step_index];
    const FaultConfig::Fault fault = this->m_fault_parameter[this->m_sm_state.active_fault_index].get_fault();
    if (step == FaultConfig::Step::SKIP) {
        this->faultManagerStateMachine_sendSignal_StepSuccessful();
        return;
    }
    // Disabled responses walk their steps without dispatching them
    if (this->m_response_parameter[response.e] != Fw::Enabled::ENABLED) {
        this->log_ACTIVITY_LO_StepSkipped(step, response, fault);
        this->m_sm_state.active_step_index++;
        this->faultManagerStateMachine_sendSignal_StepSuccessful();
        return;
    }
    this->m_sm_state.active_step = step;
    this->log_ACTIVITY_LO_StepStarted(step, response, fault);

    const FwSizeType step_index = this->stepToStepEntryIndex(step);
    bool dispatched = false;
    if (step_index != NO_ACTIVE_INDEX) {
        const StepDefinitionEntry& step_entry = this->m_step_definition_table[step_index];
        const FaultConfig::Port& port = step_entry.get_dispatchPort();
        if ((port.e < FaultConfig::Port::NUM_PORTS) &&
            this->isConnected_stepDispatchOut_OutputPort(static_cast<FwIndexType>(port.e))) {
            this->stepDispatchOut_out(static_cast<FwIndexType>(port.e), response, step, step_entry.get_context());
            dispatched = true;
        } else {
            this->log_WARNING_HI_StepPortUnconnected(step, port);
        }
    } else {
        Fw::Logger::log("[CRITICAL] FaultManager: step %d has no step definition entry\n", step.e);
    }
    if (not dispatched) {
        this->handleStepResult(Fw::Success::FAILURE);
    }
}

// ----------------------------------------------------------------------
// Implementations for internal state machine guards
// ----------------------------------------------------------------------

bool FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_guard_hasReport(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) const {
    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        const FaultResponseEntry& entry = this->m_fault_parameter[i];
        if (FaultManager::isConfiguredFault(entry.get_fault()) && (entry.get_enabled() == Fw::Enabled::ENABLED) &&
            this->m_sm_state.latched_fault_reports[entry.get_fault()]) {
            return true;
        }
    }
    return false;
}

bool FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_guard_countdownExpired(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) const {
    return this->m_sm_state.countdown == 0;
}

bool FaultManager ::Svc_FaultProtection_FaultManagerStateMachine_guard_responseDone(
    SmId smId,
    Svc_FaultProtection_FaultManagerStateMachine::Signal signal) const {
    if ((this->m_sm_state.active_response_index == NO_ACTIVE_INDEX) ||
        (this->m_sm_state.active_step_index >= FaultConfig::FAULT_RESPONSE_STEP_COUNT)) {
        return true;
    }
    const ResponseDefinitionEntry& response_entry =
        this->m_response_definition_table[this->m_sm_state.active_response_index];
    return response_entry.get_steps()[this->m_sm_state.active_step_index] == FaultConfig::Step::SKIP;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void FaultManager ::handleStepResult(const Fw::Success& status) {
    const FaultConfig::Step step = this->m_sm_state.active_step;
    const FaultConfig::Response response =
        this->m_response_definition_table[this->m_sm_state.active_response_index].get_response();
    const FaultConfig::Fault fault = this->m_fault_parameter[this->m_sm_state.active_fault_index].get_fault();
    this->m_sm_state.active_step = FaultConfig::Step::SKIP;
    this->m_sm_state.active_step_index++;

    if (status == Fw::Success::SUCCESS) {
        this->log_ACTIVITY_LO_StepCompleted(step, response, fault);
        this->faultManagerStateMachine_sendSignal_StepSuccessful();
        return;
    }
    const FaultConfig::FailureMode mode = FaultManager::isConfiguredStep(step)
                                              ? this->m_step_parameter[step.e]
                                              : FaultConfig::FailureMode(FaultConfig::FailureMode::FAULT);
    this->log_WARNING_HI_StepFailed(step, response, fault, mode);
    // The result is recorded before the signal is queued: state exit actions run ahead of transition actions
    switch (mode.e) {
        case FaultConfig::FailureMode::IGNORE:
            this->faultManagerStateMachine_sendSignal_StepSuccessful();
            break;
        case FaultConfig::FailureMode::DEFER:
            this->m_sm_state.response_result = Fw::Success::FAILURE;
            this->faultManagerStateMachine_sendSignal_StepDeferredFailure();
            break;
        case FaultConfig::FailureMode::FAULT:
        default:
            this->m_sm_state.response_result = Fw::Success::FAILURE;
            this->faultManagerStateMachine_sendSignal_StepFailed();
            break;
    }
}

void FaultManager ::cancelActiveStep() {
    const FaultConfig::Step step = this->m_sm_state.active_step;
    this->m_sm_state.active_step = FaultConfig::Step::SKIP;
    if (step == FaultConfig::Step::SKIP) {
        return;
    }
    const FwSizeType step_index = this->stepToStepEntryIndex(step);
    if (step_index != NO_ACTIVE_INDEX) {
        const FaultConfig::Port& port = this->m_step_definition_table[step_index].get_dispatchPort();
        if ((port.e < FaultConfig::Port::NUM_PORTS) &&
            this->isConnected_stepCancelOut_OutputPort(static_cast<FwIndexType>(port.e))) {
            this->log_ACTIVITY_HI_StepCancel(step);
            this->stepCancelOut_out(static_cast<FwIndexType>(port.e));
        }
    }
}

void FaultManager ::reportInternalFault(const FaultConfig::Fault& fault) {
    this->reportIn_handler(0, fault);
}

void FaultManager ::clearLatchesForResponse(const FaultConfig::Response& response) {
    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        const FaultResponseEntry& entry = this->m_fault_parameter[i];
        if (FaultManager::isConfiguredFault(entry.get_fault()) && (entry.get_response() == response) &&
            (entry.get_enabled() == Fw::Enabled::ENABLED)) {
            this->m_sm_state.latched_fault_reports[entry.get_fault()] = false;
        }
    }
}

void FaultManager ::writeTelemetry() {
    this->tlmWrite_FaultsReported(this->m_faults_reported);
    this->tlmWrite_FaultsIgnored(this->m_faults_ignored);
    this->tlmWrite_ResponsesCompleted(this->m_responses_completed);
    this->tlmWrite_ResponsesFailed(this->m_responses_failed);
}

FwSizeType FaultManager ::faultToFaultEntryIndex(const FaultConfig::Fault& fault) const {
    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        if (this->m_fault_parameter[i].get_fault() == fault) {
            return i;
        }
    }
    return NO_ACTIVE_INDEX;
}

FwSizeType FaultManager ::responseToResponseEntryIndex(const FaultConfig::Response& response) const {
    for (FwSizeType i = 0; i < ResponseDefinitionTable::SIZE; i++) {
        if (this->m_response_definition_table[i].get_response() == response) {
            return i;
        }
    }
    return NO_ACTIVE_INDEX;
}

FwSizeType FaultManager ::stepToStepEntryIndex(const FaultConfig::Step& step) const {
    for (FwSizeType i = 0; i < StepDefinitionTable::SIZE; i++) {
        if (this->m_step_definition_table[i].get_step() == step) {
            return i;
        }
    }
    return NO_ACTIVE_INDEX;
}

bool FaultManager ::isConfiguredFault(const FaultConfig::Fault& fault) {
    return fault.e < FaultConfig::Fault::NUM_FAULTS;
}

bool FaultManager ::isConfiguredResponse(const FaultConfig::Response& response) {
    return response.e < FaultConfig::Response::NUM_RESPONSES;
}

bool FaultManager ::isConfiguredStep(const FaultConfig::Step& step) {
    return step.e < FaultConfig::Step::NUM_STEPS;
}

bool FaultManager ::isValidFaultTable(const FaultResponseTable& table) {
    for (FwSizeType i = 0; i < FaultResponseTable::SIZE; i++) {
        if ((not FaultManager::isConfiguredFault(table[i].get_fault())) ||
            (not FaultManager::isConfiguredResponse(table[i].get_response()))) {
            return false;
        }
    }
    return true;
}

}  // namespace FaultProtection

}  // namespace Svc
