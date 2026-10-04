// ======================================================================
// \title  SequenceResponder.cpp
// \author mstarch
// \brief  cpp file for SequenceResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/SequenceResponder/SequenceResponder.hpp"

#include "Fw/Types/String.hpp"
#include "Fw/Types/StringUtils.hpp"
#include "Svc/Seq/SeqArgsSerializableAc.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SequenceResponder ::SequenceResponder(const char* const compName)
    : SequenceResponderComponentBase(compName),
      m_directory("."),
      m_active(false),
      m_active_response(),
      m_active_step(FaultConfig::Step::SKIP) {}

SequenceResponder ::~SequenceResponder() {}

void SequenceResponder ::configure(const Fw::StringBase& directory) {
    // The directory must leave room for the longest step name; a configuration that cannot is a deployment error
    FW_ASSERT(directory.length() < this->m_directory.getCapacity(), static_cast<FwAssertArgType>(directory.length()));
    this->m_directory = directory;
}

bool SequenceResponder ::sequenceFileName(const FaultConfig::Step& step, Fw::FileNameString& fileName) const {
    Fw::FormatStatus status = Fw::FormatStatus::SUCCESS;
#if FW_SERIALIZABLE_TO_STRING
    // Enumeration strings read "NAME (value)": the file is named by NAME alone
    Fw::String stepString;
    step.toString(stepString);
    char stepName[Fw::String::STRING_SIZE] = {};
    const FwSignedSizeType separator =
        Fw::StringUtils::substring_find(stepString.toChar(), stepString.length(), " ", 1);
    const FwSizeType nameLength = (separator >= 0) ? static_cast<FwSizeType>(separator) : stepString.length();
    (void)Fw::StringUtils::string_copy(stepName, stepString.toChar(), FW_MIN(nameLength + 1, sizeof(stepName)));
    status = fileName.format("%s/%s.seq", this->m_directory.toChar(), stepName);
#else
    status = fileName.format("%s/%" PRIu8 ".seq", this->m_directory.toChar(), static_cast<U8>(step.e));
#endif
    return status == Fw::FormatStatus::SUCCESS;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void SequenceResponder ::faultResponseCancel_handler(FwIndexType portNum) {
    const bool was_active = this->m_active;
    const FaultConfig::Response response = this->m_active_response;
    const FaultConfig::Step step = this->m_active_step;
    // Clear the active step first: the sequencer's cancellation status is then ignored rather than forwarded
    this->m_active = false;
    if (was_active) {
        this->log_ACTIVITY_HI_SequenceCanceled(step, response);
        if (this->isConnected_seqCancelOut_OutputPort(0)) {
            this->seqCancelOut_out(0);
        }
    }
}

void SequenceResponder ::faultResponseDispatch_handler(FwIndexType portNum,
                                                       const FaultConfig::Response& response,
                                                       const FaultConfig::Step& step,
                                                       const FaultConfig::Context& context) {
    if (this->m_active) {
        this->log_WARNING_HI_ResponderBusy(step, response);
        this->faultResponseComplete_out(0, Fw::Success::FAILURE, response, step);
        return;
    }
    if (not this->isConnected_seqRunOut_OutputPort(0)) {
        this->log_WARNING_HI_SequencerUnconnected();
        this->faultResponseComplete_out(0, Fw::Success::FAILURE, response, step);
        return;
    }
    Fw::FileNameString fileName;
    if (not this->sequenceFileName(step, fileName)) {
        this->log_WARNING_HI_SequenceFileNameTooLong(step);
        this->faultResponseComplete_out(0, Fw::Success::FAILURE, response, step);
        return;
    }
    this->m_active = true;
    this->m_active_response = response;
    this->m_active_step = step;
    this->log_ACTIVITY_HI_SequenceStarted(step, response, fileName);
    this->seqRunOut_out(0, fileName, Svc::SeqArgs());
}

void SequenceResponder ::seqDoneIn_handler(FwIndexType portNum,
                                           FwOpcodeType opCode,
                                           U32 cmdSeq,
                                           const Fw::CmdResponse& response) {
    const bool was_active = this->m_active;
    const FaultConfig::Response active_response = this->m_active_response;
    const FaultConfig::Step active_step = this->m_active_step;
    this->m_active = false;
    if (not was_active) {
        this->log_WARNING_LO_UnexpectedSequenceDone(response);
        return;
    }
    if (response == Fw::CmdResponse::OK) {
        this->log_ACTIVITY_HI_SequenceCompleted(active_step, active_response);
        this->faultResponseComplete_out(0, Fw::Success::SUCCESS, active_response, active_step);
    } else {
        this->log_WARNING_HI_SequenceFailed(active_step, active_response, response);
        this->faultResponseComplete_out(0, Fw::Success::FAILURE, active_response, active_step);
    }
}

}  // namespace FaultProtection

}  // namespace Svc
