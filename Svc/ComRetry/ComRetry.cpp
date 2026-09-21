// ======================================================================
// \title  ComRetry.cpp
// \author valdaarhun
// \brief  cpp file for ComRetry component implementation class
// ======================================================================

#include "Svc/ComRetry/ComRetry.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

ComRetry ::ComRetry(const char* const compName)
    : ComRetryComponentBase(compName),
      m_num_retries(3),
      m_retry_count(0),
      m_retry_state(WAITING_FOR_SEND),
      m_recover_on_sender_thread(false),
      m_bufferState(Fw::Buffer::OwnershipState::OWNED) {}

ComRetry ::~ComRetry() {}

void ComRetry::configure(U32 num_retries, bool recover_on_sender_thread) {
    this->m_num_retries = num_retries;
    this->m_recover_on_sender_thread = recover_on_sender_thread;
}

// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void ComRetry ::comStatusIn_handler(FwIndexType portNum, Fw::Success& condition) {
    this->m_mutex.lock();
    FW_ASSERT(this->m_bufferState == Fw::Buffer::OwnershipState::OWNED);

    // When waiting for send, just pass the status up the stack as the buffer should still be upstream
    if (this->m_retry_state == WAITING_FOR_SEND) {
        FW_ASSERT(!this->m_buffer.isValid());
        this->m_mutex.unlock();
        this->comStatusOut_out(0, condition);
    }
    // Nominal case where delivery of buffer is successful, and everything is passed back up the stack
    else if ((this->m_retry_state == WAITING_FOR_STATUS) && (condition == Fw::Success::SUCCESS)) {
        FW_ASSERT(this->m_buffer.isValid());
        this->finish(condition);
    }
    // When retrying, and "success" is received, this is the retry case
    else if ((this->m_retry_state == RETRYING) && (condition == Fw::Success::SUCCESS)) {
        FW_ASSERT(this->m_buffer.isValid());
        // Recovery is pulled back onto the blocked dataIn thread: only mark ready and wake it
        if (this->m_recover_on_sender_thread) {
            this->m_retry_state = RESEND_READY;
            this->m_mutex.unlock();
            this->m_condition.notify();
        } else {
            this->m_mutex.unlock();
            this->resend();
        }
    }
    // Duplicate recovery while the blocked dataIn thread is already armed to resend: nothing to do
    else if ((this->m_retry_state == RESEND_READY) && (condition == Fw::Success::SUCCESS)) {
        this->m_mutex.unlock();
    } else {
        // When a failure has been seen, it can **only** be in WAITING_FOR_STATUS state
        FW_ASSERT(this->m_retry_state == WAITING_FOR_STATUS);
        FW_ASSERT(condition == Fw::Success::FAILURE);

        // If we have retries left then switch to RETRYING, and wait for success
        if (this->m_retry_count < this->m_num_retries) {
            this->m_retry_state = RETRYING;
            this->m_mutex.unlock();
        }
        // If no retries left, pass failure back up the stack and reset state
        else {
            this->finish(condition);
        }
    }
}

void ComRetry ::dataIn_handler(FwIndexType portNum, Fw::Buffer& buffer, const ComCfg::FrameContext& context) {
    this->m_mutex.lock();
    FW_ASSERT(this->m_bufferState == Fw::Buffer::OwnershipState::OWNED);
    FW_ASSERT(this->m_retry_state == WAITING_FOR_SEND);
    this->m_bufferState = Fw::Buffer::OwnershipState::NOT_OWNED;
    this->m_retry_state = WAITING_FOR_STATUS;
    this->m_retry_count = 0;
    this->m_mutex.unlock();
    this->dataOut_out(0, buffer, context);

    if (!this->m_recover_on_sender_thread) {
        return;
    }
    // Hold the caller's thread until delivery completes, issuing every resend from this thread
    this->m_mutex.lock();
    while (this->m_retry_state != WAITING_FOR_SEND) {
        if (this->m_retry_state == RESEND_READY) {
            this->m_mutex.unlock();
            this->resend();
            this->m_mutex.lock();
        } else {
            this->m_condition.wait(this->m_mutex);
        }
    }
    this->m_mutex.unlock();
}

void ComRetry ::dataReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer, const ComCfg::FrameContext& context) {
    Os::ScopeLock lock(this->m_mutex);
    FW_ASSERT(this->m_bufferState == Fw::Buffer::OwnershipState::NOT_OWNED);
    FW_ASSERT(this->m_retry_state == WAITING_FOR_STATUS);
    this->m_bufferState = Fw::Buffer::OwnershipState::OWNED;
    this->m_buffer = buffer;
    this->m_context = context;
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void ComRetry ::resend() {
    this->m_mutex.lock();
    FW_ASSERT(this->m_buffer.isValid());
    this->m_retry_count++;
    this->m_retry_state = WAITING_FOR_STATUS;
    this->m_bufferState = Fw::Buffer::OwnershipState::NOT_OWNED;
    Fw::Buffer buffer = this->m_buffer;
    const ComCfg::FrameContext context = this->m_context;
    this->m_mutex.unlock();
    this->dataOut_out(0, buffer, context);
}

void ComRetry ::finish(Fw::Success& condition) {
    // Called with the mutex held; releases it before invoking output ports
    this->m_retry_state = WAITING_FOR_SEND;
    Fw::Buffer buffer = this->m_buffer;
    const ComCfg::FrameContext context = this->m_context;
    this->m_buffer = Fw::Buffer();  // Clear buffer
    this->m_mutex.unlock();
    this->dataReturnOut_out(0, buffer, context);
    this->comStatusOut_out(0, condition);
    if (this->m_recover_on_sender_thread) {
        this->m_condition.notify();
    }
}

}  // namespace Svc
