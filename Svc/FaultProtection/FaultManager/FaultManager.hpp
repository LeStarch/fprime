// ======================================================================
// \title  FaultManager.hpp
// \author mstarch
// \brief  hpp file for FaultManager component implementation class
// ======================================================================

#ifndef Svc_FaultProtection_FaultManager_HPP
#define Svc_FaultProtection_FaultManager_HPP

#include <atomic>

#include "Fw/Prm/PrmExternalTypes.hpp"
#include "Svc/FaultProtection/FaultConfig/FppConstantsAc.hpp"
#include "Svc/FaultProtection/FaultManager/FaultManagerComponentAc.hpp"
#include "Svc/FaultProtection/FaultManager/ResponseDefinitionTableArrayAc.hpp"
#include "Svc/FaultProtection/FaultManager/StepDefinitionTableArrayAc.hpp"

namespace Svc {

namespace FaultProtection {

class FaultManager final : public FaultManagerComponentBase, public Fw::ParamExternalDelegate {
  public:
    //! Sentinel index meaning "no active response"
    static constexpr FwSizeType NO_ACTIVE_INDEX = std::numeric_limits<FwSizeType>::max();

    /**
     * \struct GovernedState: State governed by the state machine actions. Only the latches are touched outside the
     * component's thread (by the synchronous reportIn handler), hence they alone are atomic.
     */
    struct GovernedState {
        FwSizeType countdown;              //!< Countdown for delayed response execution
        Fw::Success response_result;       //!< Result of the response execution
        FaultConfig::Fault preempted_by;   //!< Fault whose report preempts the active response
        FwSizeType active_fault_index;     //!< Index (in the fault response table) of the fault being responded to
        FwSizeType active_response_index;  //!< Index (in the response definition table) of the active response
        FwSizeType active_step_index;      //!< Index of the next step to dispatch in the active response
        FaultConfig::Step active_step;     //!< Step currently dispatched and awaiting completion (SKIP when none)
        std::atomic<bool>
            latched_fault_reports[FaultConfig::Fault::NUM_FAULTS];  //!< Faults currently latched, indexed by fault id
    };

    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct FaultManager object
    FaultManager(const char* const compName  //!< The component name
    );

    //! Destroy FaultManager object
    ~FaultManager();

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for stepCompletionIn
    //!
    //! Incoming response step completion
    void stepCompletionIn_handler(FwIndexType portNum,                    //!< The port number
                                  const Fw::Success& status,              //!< Status of the fault response
                                  const FaultConfig::Response& response,  //!< Active fault response
                                  const FaultConfig::Step& step           //!< Step of the active fault response
                                  ) override;

    //! Handler implementation for reportIn
    //!
    //! Incoming fault report. Latches the report and defers all other processing to the component's thread.
    void reportIn_handler(FwIndexType portNum,          //!< The port number
                          const FaultConfig::Fault& id  //!< Fault report identifier
                          ) override;

    //! Handler implementation for run
    //!
    //! Rate group tick driving the state machine
    void run_handler(FwIndexType portNum,  //!< The port number
                     U32 context           //!< The call order
                     ) override;

    // ----------------------------------------------------------------------
    // Handler implementations for internal ports
    // ----------------------------------------------------------------------

    //! Handler implementation for handleReport
    //!
    //! Announces a latched report, discards disabled reports, and preempts the active response when warranted
    void handleReport_internalInterfaceHandler(const FaultConfig::Fault& fault, bool latched) override;

    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command SET_FAULT_ENABLED
    void SET_FAULT_ENABLED_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                      U32 cmdSeq,           //!< The command sequence number
                                      const FaultConfig::Fault& fault,
                                      const Fw::Enabled& enabled) override;

    //! Handler implementation for command SET_RESPONSE_ENABLED
    void SET_RESPONSE_ENABLED_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                         U32 cmdSeq,           //!< The command sequence number
                                         const FaultConfig::Response& response,
                                         const Fw::Enabled& enabled) override;

    //! Handler implementation for command UPDATE_STEP_FAILURE_MODE
    void UPDATE_STEP_FAILURE_MODE_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                                             U32 cmdSeq,           //!< The command sequence number
                                             const FaultConfig::Step& step,
                                             const FaultConfig::FailureMode& failureMode) override;

    // ----------------------------------------------------------------------
    // Implementations for external parameter delegate serialization
    // ----------------------------------------------------------------------

    //! Deserialize a parameter from a parameter buffer
    Fw::SerializeStatus deserializeParam(const FwPrmIdType base_id,     //!< The component base parameter ID
                                         const FwPrmIdType local_id,    //!< The parameter local ID
                                         const Fw::ParamValid prmStat,  //!< The parameter validity status
                                         Fw::SerialBufferBase& buff     //!< The buffer containing the parameter
                                         ) override;

    //! Serialize a parameter into a parameter buffer
    Fw::SerializeStatus serializeParam(const FwPrmIdType base_id,   //!< The component base parameter ID
                                       const FwPrmIdType local_id,  //!< The parameter local ID
                                       Fw::SerialBufferBase& buff   //!< The buffer to serialize the parameter into
    ) const override;

    // ----------------------------------------------------------------------
    // Implementations for internal state machine actions
    // ----------------------------------------------------------------------

    //! Implementation for action startCountdown of state machine Svc_FaultProtection_FaultManagerStateMachine
    void Svc_FaultProtection_FaultManagerStateMachine_action_startCountdown(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action decrementCountdown of state machine Svc_FaultProtection_FaultManagerStateMachine
    void Svc_FaultProtection_FaultManagerStateMachine_action_decrementCountdown(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action selectResponse of state machine Svc_FaultProtection_FaultManagerStateMachine
    void Svc_FaultProtection_FaultManagerStateMachine_action_selectResponse(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action completeResponse of state machine Svc_FaultProtection_FaultManagerStateMachine
    void Svc_FaultProtection_FaultManagerStateMachine_action_completeResponse(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    //! Implementation for action dispatchStep of state machine Svc_FaultProtection_FaultManagerStateMachine
    void Svc_FaultProtection_FaultManagerStateMachine_action_dispatchStep(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
        ) override;

    // ----------------------------------------------------------------------
    // Implementations for internal state machine guards
    // ----------------------------------------------------------------------

    //! Implementation for guard hasReport of state machine Svc_FaultProtection_FaultManagerStateMachine
    bool Svc_FaultProtection_FaultManagerStateMachine_guard_hasReport(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard countdownExpired of state machine Svc_FaultProtection_FaultManagerStateMachine
    bool Svc_FaultProtection_FaultManagerStateMachine_guard_countdownExpired(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
    ) const override;

    //! Implementation for guard responseDone of state machine Svc_FaultProtection_FaultManagerStateMachine
    bool Svc_FaultProtection_FaultManagerStateMachine_guard_responseDone(
        SmId smId,                                                   //!< The state machine id
        Svc_FaultProtection_FaultManagerStateMachine::Signal signal  //!< The signal
    ) const override;

    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Record the result of the active step and queue the matching state machine signal
    void handleStepResult(const Fw::Success& status);

    //! Cancel the step awaiting completion (if any) through its responder's cancel port
    void cancelActiveStep();

    //! Report a fault from within the component (used for FAULT_RESPONSE_FAILURE)
    void reportInternalFault(const FaultConfig::Fault& fault);

    //! Clear the latch of every enabled fault mapped to the given response
    void clearLatchesForResponse(const FaultConfig::Response& response);

    //! Write all telemetry channels
    void writeTelemetry();

    //! Look up the fault response table index for a fault. Returns NO_ACTIVE_INDEX when not found.
    FwSizeType faultToFaultEntryIndex(const FaultConfig::Fault& fault) const;

    //! Look up the response definition table index for a response. Returns NO_ACTIVE_INDEX when not found.
    FwSizeType responseToResponseEntryIndex(const FaultConfig::Response& response) const;

    //! Look up the step definition table index for a step. Returns NO_ACTIVE_INDEX when not found.
    FwSizeType stepToStepEntryIndex(const FaultConfig::Step& step) const;

    //! Is the given fault id a valid, configured fault (i.e. less than NUM_FAULTS)
    static bool isConfiguredFault(const FaultConfig::Fault& fault);

    //! Is the given response a valid, configured response (i.e. less than NUM_RESPONSES)
    static bool isConfiguredResponse(const FaultConfig::Response& response);

    //! Is the given step a valid, configured step (i.e. less than NUM_STEPS)
    static bool isConfiguredStep(const FaultConfig::Step& step);

    //! Validate a fault response table before accepting it
    static bool isValidFaultTable(const FaultResponseTable& table);

    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! Fault response table (per-fault enabled, precedence, response); backs FAULT_RESPONSE_TABLE
    FaultResponseTable m_fault_parameter;
    //! Per-response enabled flags; backs RESPONSE_TABLE
    ResponsesEnabled m_response_parameter;
    //! Per-step failure modes; backs STEP_TABLE
    StepFailureModes m_step_parameter;

    //! Response definitions (immutable configuration)
    const ResponseDefinitionTable m_response_definition_table;
    //! Step definitions (immutable configuration)
    const StepDefinitionTable m_step_definition_table;

    //! State governed by the state machine
    GovernedState m_sm_state;

    FwSizeType m_faults_reported;      //!< Count of faults reported and latched
    FwSizeType m_faults_ignored;       //!< Count of faults reported but ignored
    FwSizeType m_responses_completed;  //!< Count of responses completed successfully
    FwSizeType m_responses_failed;     //!< Count of responses that failed
};

}  // namespace FaultProtection

}  // namespace Svc

#endif
