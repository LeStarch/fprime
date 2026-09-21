// ======================================================================
// \title  ComRetryTester.hpp
// \author valdaarhun
// \brief  hpp file for ComRetry test harness implementation class
// ======================================================================

#ifndef Svc_ComRetryTester_HPP
#define Svc_ComRetryTester_HPP

#include "ComRetryGTestBase.hpp"
#include "Os/CountingSemaphore.hpp"
#include "Svc/ComRetry/ComRetry.hpp"

#define BUFFER_LENGTH 3u
#define DATA_A {0xad, 0xbe, 0xde}
#define DATA_B {0xde, 0xef, 0xf0}

namespace Svc {

class ComRetryTester final : public ComRetryGTestBase {
  public:
    // ----------------------------------------------------------------------
    // Constants
    // ----------------------------------------------------------------------

    // Maximum size of histories storing events, telemetry, and port outputs
    static const FwSizeType MAX_HISTORY_SIZE = 10;

    // Instance ID supplied to the component instance under test
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

  public:
    // ----------------------------------------------------------------------
    // Construction and destruction
    // ----------------------------------------------------------------------

    //! Construct object ComRetryTester
    ComRetryTester();

    //! Destroy object ComRetryTester
    ~ComRetryTester();

  public:
    // ----------------------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------------------
    void configure(U32 num_retries, bool recover_on_sender_thread = false);

    void receiveBuffer(Fw::Buffer& buffer, ComCfg::FrameContext& context);

    void checkDataOut(FwIndexType expectedIndex, U8* expectedData, FwSizeType expectedDataSize);

    // ----------------------------------------------------------------------
    // Tests
    // ----------------------------------------------------------------------

    void testNullBuffer();

    void testBufferSend();

    void testBufferRetry();

    void testBufferRetryTillFailure();

    //! Recovery SUCCESS from another thread only wakes the blocked dataIn caller, which performs the resend
    void testRecoverOnSenderThread();

    //! Retry exhaustion releases the blocked dataIn caller with FAILURE
    void testRecoverOnSenderThreadTillFailure();

    //! Nested synchronous statuses from inside dataOut complete without blocking
    void testRecoverOnSenderThreadNested();

  private:
    // ----------------------------------------------------------------------
    // Handler overrides
    // ----------------------------------------------------------------------

    //! Records dataOut and flags any resend issued from within a comStatusIn call
    void from_dataOut_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) override;

    //! Sender task: invokes dataIn once and signals completion
    static void senderTask(void* argument);

    //! Deliver a status from the test thread, recording that a nested dataOut would be a violation
    void deliverStatus(Fw::Success status);

    //! Wait for the next dataOut emission
    bool awaitDataOut();

  private:
    // ----------------------------------------------------------------------
    // Helper functions
    // ----------------------------------------------------------------------

    //! Connect ports
    void connectPorts();

    //! Initialize components
    void initComponents();

  private:
    // ----------------------------------------------------------------------
    // Member variables
    // ----------------------------------------------------------------------

    //! The component under test
    ComRetry component;

    Fw::Buffer m_senderBuffer;             //!< Buffer delivered by the sender task
    ComCfg::FrameContext m_senderContext;  //!< Context delivered by the sender task
    Os::CountingSemaphore m_dataOutSeen;   //!< Posted on every dataOut
    Os::CountingSemaphore m_senderDone;    //!< Posted when the sender task's dataIn returns
    bool m_inStatusCall;                   //!< True while the test thread is inside comStatusIn
    U32 m_dataOutOnStatusThread;           //!< Count of dataOut emitted from within comStatusIn
    bool m_nestedReply;                    //!< Reply FAILURE-then-SUCCESS synchronously from dataOut
    U32 m_nestedFailures;                  //!< Remaining synchronous FAILURE replies
};

}  // namespace Svc

#endif
