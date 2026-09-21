// ======================================================================
// \title  ComRetry.hpp
// \author valdaarhun
// \brief  hpp file for ComRetry component implementation class
// ======================================================================

#ifndef Svc_ComRetry_HPP
#define Svc_ComRetry_HPP

#include "Os/Condition.hpp"
#include "Os/Mutex.hpp"
#include "Svc/ComRetry/ComRetryComponentAc.hpp"

namespace Svc {

class ComRetry final : public ComRetryComponentBase {
    //! State of buffer delivery
    enum RetryState { WAITING_FOR_STATUS, WAITING_FOR_SEND, RETRYING, RESEND_READY };

  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct ComRetry object
    ComRetry(const char* const compName  //!< The component name
    );

    //! Destroy ComRetry object
    ~ComRetry();

    //! Configure the number of retries and where retries execute
    //!
    //! When `recover_on_sender_thread` is true, `dataIn` blocks on an Os::ConditionVariable until the buffer is
    //! delivered or retries are exhausted, and every resend is issued from the `dataIn` caller's thread. A recovery
    //! SUCCESS arriving on `comStatusIn` from any other thread (e.g. a radio receive thread) only notifies the waiter.
    void configure(U32 num_retries,                       //!< Number of retries allowed
                   bool recover_on_sender_thread = false  //!< Pull retries back onto the dataIn caller's thread
    );

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for comStatusIn
    //!
    //! Resend last delivered message on failure
    void comStatusIn_handler(FwIndexType portNum,    //!< The port number
                             Fw::Success& condition  //!< Condition success/failure
                             ) override;

    //! Handler implementation for dataIn
    //!
    //! Port to receive data in a Fw::Buffer with optional context
    void dataIn_handler(FwIndexType portNum,  //!< The port number
                        Fw::Buffer& data,
                        const ComCfg::FrameContext& context) override;

    //! Handler implementation for dataReturnIn
    //!
    //! Buffer coming from a deallocate call in a ComDriver component
    void dataReturnIn_handler(FwIndexType portNum,  //!< The port number
                              Fw::Buffer& data,
                              const ComCfg::FrameContext& context) override;

  private:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------

    //! Issue a resend of the stored buffer; mutex must not be held
    void resend();

    //! Return the stored buffer upstream and forward the final status; called with the mutex held, releases it
    void finish(Fw::Success& condition);

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------
    U32 m_num_retries;                         //!< Maximum number of retries
    U32 m_retry_count;                         //!< Track number of attempted retries
    RetryState m_retry_state;                  //!< Track current retry state
    bool m_recover_on_sender_thread;           //!< Resend from the dataIn caller's thread
    ComCfg::FrameContext m_context;            //!< Context for the current frame
    Fw::Buffer m_buffer;                       //!< Store incoming buffer
    Fw::Buffer::OwnershipState m_bufferState;  //!< Track ownership of stored buffer
    Os::Mutex m_mutex;                         //!< Guards state shared between dataIn and comStatusIn threads
    Os::ConditionVariable m_condition;         //!< Wakes the blocked dataIn thread on a status change
};

}  // namespace Svc

#endif
