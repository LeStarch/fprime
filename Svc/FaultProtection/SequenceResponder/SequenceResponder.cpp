// ======================================================================
// \title  SequenceResponder.cpp
// \author mstarch
// \brief  cpp file for SequenceResponder component implementation class
// ======================================================================

#include "Svc/FaultProtection/SequenceResponder/SequenceResponder.hpp"

#include "Fw/Types/String.hpp"
#include "Svc/Seq/SeqArgsSerializableAc.hpp"

namespace Svc {

namespace FaultProtection {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

SequenceResponder ::SequenceResponder(const char* const compName)
    : SequenceResponderComponentBase(compName),
      m_directory("."),
      m_lock(),
      m_active(false),
      m_active_response(),
      m_active_step(FaultConfig::Step::SKIP) {}

SequenceResponder ::~SequenceResponder() {}

void SequenceResponder ::configure(const char* directory) {
    FW_ASSERT(directory != nullptr);
    this->m_directory = directory;
}

void SequenceResponder ::sequenceFileName(const FaultConfig::Step& step, Fw::FileNameString& fileName) const {
#if FW_SERIALIZABLE_TO_STRING
    // Enumeration strings read "NAME (value)": the file is named by NAME alone (names never contain spaces)
    Fw::String stepString;
    step.toString(stepString);
    char stepName[Fw::String::STRING_SIZE];
    FwSizeType length = 0;
    for (; (length < (sizeof(stepName) - 1)) && (stepString.toChar()[length] != '\0') &&
           (stepString.toChar()[length] != ' ');
         length++) {
        stepName[length] = stepString.toChar()[length];
    }
    stepName[length] = '\0';
    (void)fileName.format("%s/%s.seq", this->m_directory.toChar(), stepName);
#else
    (void)fileName.format("%s/%" PRIu8 ".seq", this->m_directory.toChar(), static_cast<U8>(step.e));
#endif
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void SequenceResponder ::faultResponseCancel_handler(FwIndexType portNum) {
    this->m_lock.lock();
    const bool was_active = this->m_active;
    const FaultConfig::Response response = this->m_active_response;
    const FaultConfig::Step step = this->m_active_step;
    // Clear the active step first: the sequencer's cancellation status is then ignored rather than forwarded
    this->m_active = false;
    this->m_lock.unlock();
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
    this->m_lock.lock();
    const bool busy = this->m_active;
    if (not busy) {
        this->m_active = true;
        this->m_active_response = response;
        this->m_active_step = step;
    }
    this->m_lock.unlock();
    if (busy) {
        this->log_WARNING_HI_ResponderBusy(step, response);
        this->faultResponseComplete_out(0, Fw::Success::FAILURE, response, step);
        return;
    }
    if (not this->isConnected_seqRunOut_OutputPort(0)) {
        this->m_lock.lock();
        this->m_active = false;
        this->m_lock.unlock();
        this->log_WARNING_HI_SequencerUnconnected();
        this->faultResponseComplete_out(0, Fw::Success::FAILURE, response, step);
        return;
    }
    Fw::FileNameString fileName;
    this->sequenceFileName(step, fileName);
    this->log_ACTIVITY_HI_SequenceStarted(step, response, fileName);
    this->seqRunOut_out(0, fileName, Svc::SeqArgs());
}

void SequenceResponder ::seqDoneIn_handler(FwIndexType portNum,
                                           FwOpcodeType opCode,
                                           U32 cmdSeq,
                                           const Fw::CmdResponse& response) {
    this->m_lock.lock();
    const bool was_active = this->m_active;
    const FaultConfig::Response active_response = this->m_active_response;
    const FaultConfig::Step active_step = this->m_active_step;
    this->m_active = false;
    this->m_lock.unlock();
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
