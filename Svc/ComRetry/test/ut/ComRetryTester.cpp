// ======================================================================
// \title  ComRetryTester.cpp
// \author valdaarhun
// \brief  cpp file for ComRetry test harness implementation class
// ======================================================================

#include "ComRetryTester.hpp"

#include "Os/Task.hpp"

namespace {
const Fw::TimeInterval TEST_TIMEOUT(5, 0);
const Fw::TimeInterval SETTLE_DELAY(0, 50000);
//! True on the thread currently inside a deliverStatus() call
thread_local bool t_inStatusCall = false;
}  // namespace

namespace Svc {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

ComRetryTester ::ComRetryTester()
    : ComRetryGTestBase("ComRetryTester", ComRetryTester::MAX_HISTORY_SIZE),
      component("ComRetry"),
      m_dataOutSeen(0),
      m_senderDone(0),
      m_dataOutOnStatusThread(0),
      m_nestedReply(false),
      m_nestedFailures(0) {
    this->initComponents();
    this->connectPorts();
}

ComRetryTester ::~ComRetryTester() {}

void ComRetryTester ::configure(U32 num_retries = 1, bool recover_on_sender_thread) {
    component.configure(num_retries, recover_on_sender_thread);
}

void ComRetryTester ::from_dataOut_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) {
    if (t_inStatusCall) {
        this->m_dataOutOnStatusThread++;
    }
    this->pushFromPortEntry_dataOut(data, context);
    if (this->m_nestedReply) {
        // Emulate a passive adapter answering synchronously from inside dataOut
        Fw::Success status = Fw::Success::SUCCESS;
        if (this->m_nestedFailures > 0) {
            this->m_nestedFailures--;
            status = Fw::Success::FAILURE;
        }
        this->invoke_to_dataReturnIn(0, data, context);
        this->invoke_to_comStatusIn(0, status);
    }
    (void)this->m_dataOutSeen.post();
}

void ComRetryTester ::senderTask(void* argument) {
    ComRetryTester* tester = static_cast<ComRetryTester*>(argument);
    tester->invoke_to_dataIn(0, tester->m_senderBuffer, tester->m_senderContext);
    (void)tester->m_senderDone.post();
}

void ComRetryTester ::deliverStatus(Fw::Success status) {
    t_inStatusCall = true;
    this->invoke_to_comStatusIn(0, status);
    t_inStatusCall = false;
}

bool ComRetryTester ::awaitDataOut() {
    return this->m_dataOutSeen.waitTimeout(TEST_TIMEOUT) == Os::CountingSemaphore::Status::OP_OK;
}

void ComRetryTester ::receiveBuffer(Fw::Buffer& buffer, ComCfg::FrameContext& context) {
    invoke_to_dataIn(0, buffer, context);
    invoke_to_dataReturnIn(0, buffer, context);
}

void ComRetryTester ::checkDataOut(FwIndexType expectedIndex, U8* expectedData, FwSizeType expectedDataSize) {
    Fw::Buffer emittedBuffer = this->fromPortHistory_dataOut->at(expectedIndex).data;
    ASSERT_EQ(expectedDataSize, emittedBuffer.getSize());
    for (FwSizeType i = 0; i < expectedDataSize; i++) {
        ASSERT_EQ(emittedBuffer.getData()[i], expectedData[i]);
    }
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void ComRetryTester ::testNullBuffer() {
    Fw::Success state = Fw::Success::SUCCESS;
    invoke_to_comStatusIn(0, state);
    ASSERT_from_comStatusOut(0, state);

    state = Fw::Success::FAILURE;
    invoke_to_comStatusIn(0, state);
    ASSERT_from_comStatusOut(1, state);
}

void ComRetryTester ::testBufferSend() {
    U8 data_a[BUFFER_LENGTH] = DATA_A;
    U8 data_b[BUFFER_LENGTH] = DATA_B;
    Fw::Buffer buffer_a(&data_a[0], sizeof(data_a));
    Fw::Buffer buffer_b(&data_b[0], sizeof(data_b));
    ComCfg::FrameContext nullContext;
    Fw::Success state = Fw::Success::SUCCESS;
    configure();

    receiveBuffer(buffer_a, nullContext);
    invoke_to_comStatusIn(0, state);
    ASSERT_from_dataReturnOut(0, buffer_a, nullContext);
    ASSERT_from_comStatusOut(0, state);

    receiveBuffer(buffer_b, nullContext);
    invoke_to_comStatusIn(0, state);
    ASSERT_from_dataReturnOut(1, buffer_b, nullContext);
    ASSERT_from_comStatusOut(1, state);

    checkDataOut(0, buffer_a.getData(), buffer_a.getSize());
    checkDataOut(1, buffer_b.getData(), buffer_b.getSize());
}

void ComRetryTester ::testBufferRetry() {
    U8 data_a[BUFFER_LENGTH] = DATA_A;
    U8 data_b[BUFFER_LENGTH] = DATA_B;
    Fw::Buffer buffer_a(&data_a[0], sizeof(data_a));
    Fw::Buffer buffer_b(&data_b[0], sizeof(data_b));
    ComCfg::FrameContext nullContext;
    Fw::Success state = Fw::Success::FAILURE;
    configure();

    receiveBuffer(buffer_a, nullContext);
    invoke_to_comStatusIn(0, state);  // First delivery is a failure
    state = Fw::Success::SUCCESS;
    invoke_to_comStatusIn(0, state);  // Downstream component is ready to receive buffer
    invoke_to_dataReturnIn(0, buffer_a, nullContext);
    invoke_to_comStatusIn(0, state);  // Redelivery is successful

    ASSERT_from_dataReturnOut(0, buffer_a, nullContext);
    ASSERT_from_comStatusOut(0, state);

    receiveBuffer(buffer_b, nullContext);
    invoke_to_comStatusIn(0, state);
    ASSERT_from_dataReturnOut(1, buffer_b, nullContext);
    ASSERT_from_comStatusOut(1, state);

    checkDataOut(0, buffer_a.getData(), buffer_a.getSize());
    checkDataOut(1, buffer_a.getData(), buffer_a.getSize());
    checkDataOut(2, buffer_b.getData(), buffer_b.getSize());
}

void ComRetryTester ::testBufferRetryTillFailure() {
    U8 data_a[BUFFER_LENGTH] = DATA_A;
    U8 data_b[BUFFER_LENGTH] = DATA_B;
    Fw::Buffer buffer_a(&data_a[0], sizeof(data_a));
    Fw::Buffer buffer_b(&data_b[0], sizeof(data_b));
    ComCfg::FrameContext nullContext;
    Fw::Success failure = Fw::Success::FAILURE;
    Fw::Success success = Fw::Success::SUCCESS;

    FwIndexType num_retries = 3;  // This is also the default number of retries

    receiveBuffer(buffer_a, nullContext);
    invoke_to_comStatusIn(0, failure);
    checkDataOut(0, buffer_a.getData(), buffer_a.getSize());

    for (FwIndexType i = 1; i <= num_retries; i++) {
        invoke_to_comStatusIn(0, success);
        invoke_to_dataReturnIn(0, buffer_a, nullContext);
        invoke_to_comStatusIn(0, failure);
        checkDataOut(i, buffer_a.getData(), buffer_a.getSize());
    }

    ASSERT_from_dataReturnOut(0, buffer_a, nullContext);
    ASSERT_from_comStatusOut(0, failure);

    receiveBuffer(buffer_b, nullContext);
    invoke_to_comStatusIn(0, success);
    ASSERT_from_dataReturnOut(1, buffer_b, nullContext);
    ASSERT_from_comStatusOut(1, success);
    checkDataOut(num_retries + 1, buffer_b.getData(), buffer_b.getSize());
}

void ComRetryTester ::testRecoverOnSenderThread() {
    U8 data_a[BUFFER_LENGTH] = DATA_A;
    this->m_senderBuffer = Fw::Buffer(&data_a[0], sizeof(data_a));
    configure(3, true);

    Os::Task sender;
    Os::Task::Arguments arguments(Fw::String("ComRetrySender"), ComRetryTester::senderTask, this);
    ASSERT_EQ(Os::Task::Status::OP_OK, sender.start(arguments));

    // Initial delivery is issued by the sender task and fails
    ASSERT_TRUE(awaitDataOut());
    invoke_to_dataReturnIn(0, this->m_senderBuffer, this->m_senderContext);
    deliverStatus(Fw::Success::FAILURE);

    // Sender remains blocked while waiting for recovery
    ASSERT_EQ(Os::CountingSemaphore::Status::ERROR_TIMEOUT, this->m_senderDone.waitTimeout(SETTLE_DELAY));
    ASSERT_EQ(1, this->fromPortHistory_dataOut->size());

    // Recovery SUCCESS from this thread must not resend inline; the resend comes from the sender task
    deliverStatus(Fw::Success::SUCCESS);
    ASSERT_TRUE(awaitDataOut());
    ASSERT_EQ(0, this->m_dataOutOnStatusThread);
    ASSERT_EQ(2, this->fromPortHistory_dataOut->size());
    ASSERT_EQ(Os::CountingSemaphore::Status::ERROR_TIMEOUT, this->m_senderDone.waitTimeout(SETTLE_DELAY));

    // Redelivery succeeds and releases the sender
    invoke_to_dataReturnIn(0, this->m_senderBuffer, this->m_senderContext);
    deliverStatus(Fw::Success::SUCCESS);
    ASSERT_EQ(Os::CountingSemaphore::Status::OP_OK, this->m_senderDone.waitTimeout(TEST_TIMEOUT));
    ASSERT_EQ(Os::Task::Status::OP_OK, sender.join());

    ASSERT_from_dataReturnOut(0, this->m_senderBuffer, this->m_senderContext);
    ASSERT_from_comStatusOut(0, Fw::Success(Fw::Success::SUCCESS));
    checkDataOut(0, data_a, sizeof(data_a));
    checkDataOut(1, data_a, sizeof(data_a));
}

void ComRetryTester ::testRecoverOnSenderThreadTillFailure() {
    U8 data_a[BUFFER_LENGTH] = DATA_A;
    this->m_senderBuffer = Fw::Buffer(&data_a[0], sizeof(data_a));
    const U32 num_retries = 2;
    configure(num_retries, true);

    Os::Task sender;
    Os::Task::Arguments arguments(Fw::String("ComRetrySender"), ComRetryTester::senderTask, this);
    ASSERT_EQ(Os::Task::Status::OP_OK, sender.start(arguments));

    ASSERT_TRUE(awaitDataOut());
    invoke_to_dataReturnIn(0, this->m_senderBuffer, this->m_senderContext);
    deliverStatus(Fw::Success::FAILURE);
    for (U32 i = 0; i < num_retries; i++) {
        deliverStatus(Fw::Success::SUCCESS);
        ASSERT_TRUE(awaitDataOut());
        invoke_to_dataReturnIn(0, this->m_senderBuffer, this->m_senderContext);
        deliverStatus(Fw::Success::FAILURE);
    }
    ASSERT_EQ(Os::CountingSemaphore::Status::OP_OK, this->m_senderDone.waitTimeout(TEST_TIMEOUT));
    ASSERT_EQ(Os::Task::Status::OP_OK, sender.join());

    ASSERT_EQ(0, this->m_dataOutOnStatusThread);
    ASSERT_EQ(num_retries + 1, this->fromPortHistory_dataOut->size());
    ASSERT_from_dataReturnOut(0, this->m_senderBuffer, this->m_senderContext);
    ASSERT_from_comStatusOut(0, Fw::Success(Fw::Success::FAILURE));
}

void ComRetryTester ::testRecoverOnSenderThreadNested() {
    U8 data_a[BUFFER_LENGTH] = DATA_A;
    this->m_senderBuffer = Fw::Buffer(&data_a[0], sizeof(data_a));
    this->m_nestedReply = true;
    this->m_nestedFailures = 1;
    configure(3, true);

    Os::Task sender;
    Os::Task::Arguments arguments(Fw::String("ComRetrySender"), ComRetryTester::senderTask, this);
    ASSERT_EQ(Os::Task::Status::OP_OK, sender.start(arguments));

    // First attempt is refused synchronously (adapter busy); sender blocks awaiting recovery
    ASSERT_TRUE(awaitDataOut());
    ASSERT_EQ(Os::CountingSemaphore::Status::ERROR_TIMEOUT, this->m_senderDone.waitTimeout(SETTLE_DELAY));

    // Recovery from this thread; the resend is answered synchronously with SUCCESS on the sender thread
    deliverStatus(Fw::Success::SUCCESS);
    ASSERT_TRUE(awaitDataOut());
    ASSERT_EQ(Os::CountingSemaphore::Status::OP_OK, this->m_senderDone.waitTimeout(TEST_TIMEOUT));
    ASSERT_EQ(Os::Task::Status::OP_OK, sender.join());

    ASSERT_EQ(0, this->m_dataOutOnStatusThread);
    ASSERT_EQ(2, this->fromPortHistory_dataOut->size());
    ASSERT_from_dataReturnOut(0, this->m_senderBuffer, this->m_senderContext);
    ASSERT_from_comStatusOut(0, Fw::Success(Fw::Success::SUCCESS));
}

}  // namespace Svc
