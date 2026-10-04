// ======================================================================
// \title  MonitoredCounterTester.cpp
// \author mstarch
// \brief  cpp file for MonitoredCounter component test harness implementation class
// ======================================================================

#include "MonitoredCounterTester.hpp"

namespace Ref {

namespace {
const FaultConfig::Fault COUNTER_HIGH(FaultConfig::Fault::COUNTER_HIGH);
const MonitorColor BLACK(MonitorColor::BLACK);
const MonitorColor GREEN(MonitorColor::GREEN);
const MonitorColor YELLOW(MonitorColor::YELLOW);
const MonitorColor RED(MonitorColor::RED);
}  // namespace

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

MonitoredCounterTester ::MonitoredCounterTester()
    : MonitoredCounterGTestBase("MonitoredCounterTester", MonitoredCounterTester::MAX_HISTORY_SIZE),
      component("MonitoredCounter"),
      m_cycles(0) {
    this->initComponents();
    this->connectPorts();
    this->component.loadParameters();
}

MonitoredCounterTester ::~MonitoredCounterTester() {
    this->component.deinit();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------

void MonitoredCounterTester ::testCounting() {
    this->cycleTo(DEFAULT_COUNT_THRESHOLD);
    this->assertCounts(DEFAULT_COUNT_THRESHOLD, 0);
    ASSERT_EVENTS_SIZE(0);
    ASSERT_TLM_Monitor_SIZE(0);
    ASSERT_from_faultOut_SIZE(0);
}

void MonitoredCounterTester ::testFault() {
    this->cycleTo(DEFAULT_COUNT_THRESHOLD + 1);
    this->assertCounts(DEFAULT_COUNT_THRESHOLD + 1, 1);
    ASSERT_EVENTS_SIZE(0);
    this->clearHistory();

    // Local threshold: YELLOW with a warning
    this->cycleTo(DEFAULT_COUNT_THRESHOLD + DEFAULT_LOCAL_ERROR_THRESHOLD);
    this->assertCounts(DEFAULT_COUNT_THRESHOLD + DEFAULT_LOCAL_ERROR_THRESHOLD, DEFAULT_LOCAL_ERROR_THRESHOLD);
    ASSERT_EVENTS_MonitorColorChanged_SIZE(1);
    ASSERT_EVENTS_MonitorColorChanged(0, GREEN, YELLOW, DEFAULT_COUNT_THRESHOLD + DEFAULT_LOCAL_ERROR_THRESHOLD,
                                      DEFAULT_LOCAL_ERROR_THRESHOLD);
    ASSERT_EVENTS_CountHighWarning_SIZE(1);
    ASSERT_EVENTS_CountHighFault_SIZE(0);
    ASSERT_TLM_Monitor_SIZE(1);
    ASSERT_TLM_Monitor(0, YELLOW);
    ASSERT_from_faultOut_SIZE(0);
    this->clearHistory();

    // System threshold: RED with the fault reported
    this->cycleTo(DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD);
    this->assertCounts(DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD, DEFAULT_SYSTEM_ERROR_THRESHOLD);
    ASSERT_EVENTS_MonitorColorChanged_SIZE(1);
    ASSERT_EVENTS_MonitorColorChanged(0, YELLOW, RED, DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD,
                                      DEFAULT_SYSTEM_ERROR_THRESHOLD);
    ASSERT_EVENTS_CountHighFault_SIZE(1);
    ASSERT_EVENTS_CountHighFault(0, DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD,
                                 DEFAULT_SYSTEM_ERROR_THRESHOLD);
    ASSERT_TLM_Monitor(0, RED);
    ASSERT_from_faultOut_SIZE(1);
    ASSERT_from_faultOut(0, COUNTER_HIGH);
    this->clearHistory();

    // Further cycles stay RED without repeating the report: FaultManager latches it
    this->cycle(3);
    this->assertCounts(DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD + 3,
                       DEFAULT_SYSTEM_ERROR_THRESHOLD + 3);
    ASSERT_EVENTS_SIZE(0);
    ASSERT_TLM_Monitor_SIZE(0);
    ASSERT_from_faultOut_SIZE(0);
}

void MonitoredCounterTester ::testResetRecovers() {
    this->cycleTo(DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD);
    ASSERT_from_faultOut_SIZE(1);
    this->clearHistory();

    // The corrective action: the count is reset
    this->resetCount();
    ASSERT_EVENTS_CountReset_SIZE(1);
    ASSERT_EVENTS_CountReset(0, DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD);
    ASSERT_TLM_Count_SIZE(1);
    ASSERT_TLM_Count(0, 0);
    this->clearHistory();

    // The next passing cycle recovers the monitor
    this->cycle(1);
    this->assertCounts(1, DEFAULT_SYSTEM_ERROR_THRESHOLD - 1);
    ASSERT_EVENTS_MonitorColorChanged_SIZE(1);
    ASSERT_EVENTS_MonitorColorChanged(0, RED, GREEN, 1, DEFAULT_SYSTEM_ERROR_THRESHOLD - 1);
    ASSERT_TLM_Monitor(0, GREEN);
    this->clearHistory();

    // Errors decay to zero while passing
    this->cycle(DEFAULT_SYSTEM_ERROR_THRESHOLD);
    this->assertCounts(1 + DEFAULT_SYSTEM_ERROR_THRESHOLD, 0);
    this->clearHistory();

    // A new excursion reports the fault again
    this->cycleTo(DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD);
    ASSERT_EVENTS_CountHighFault_SIZE(1);
    ASSERT_from_faultOut_SIZE(1);
    ASSERT_from_faultOut(0, COUNTER_HIGH);
}

void MonitoredCounterTester ::testMonitoringDisabled() {
    this->setMonitoring(Fw::Enabled::DISABLED);
    ASSERT_EVENTS_MonitoringSet_SIZE(1);
    ASSERT_EVENTS_MonitoringSet(0, Fw::Enabled::DISABLED);
    this->clearHistory();

    // Counting continues, the monitor is BLACK and errors are not counted even above the threshold
    this->cycle(1);
    ASSERT_EVENTS_MonitorColorChanged_SIZE(1);
    ASSERT_EVENTS_MonitorColorChanged(0, GREEN, BLACK, 1, 0);
    ASSERT_TLM_Monitor(0, BLACK);
    this->clearHistory();
    this->cycleTo(DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD);
    this->assertCounts(DEFAULT_COUNT_THRESHOLD + DEFAULT_SYSTEM_ERROR_THRESHOLD, 0);
    ASSERT_EVENTS_SIZE(0);
    ASSERT_from_faultOut_SIZE(0);

    // Re-enabled: the monitor resumes evaluating the test
    this->resetCount();
    this->setMonitoring(Fw::Enabled::ENABLED);
    this->clearHistory();
    this->cycle(1);
    this->assertCounts(1, 0);
    ASSERT_EVENTS_MonitorColorChanged_SIZE(1);
    ASSERT_EVENTS_MonitorColorChanged(0, BLACK, GREEN, 1, 0);
}

void MonitoredCounterTester ::testParameters() {
    const U32 countThreshold = 2;
    const U32 localThreshold = 1;
    const U32 systemThreshold = 2;
    this->paramSet_COUNT_THRESHOLD(countThreshold, Fw::ParamValid::VALID);
    this->paramSet_LOCAL_ERROR_THRESHOLD(localThreshold, Fw::ParamValid::VALID);
    this->paramSet_SYSTEM_ERROR_THRESHOLD(systemThreshold, Fw::ParamValid::VALID);
    this->component.loadParameters();

    this->cycleTo(countThreshold + localThreshold);
    this->assertCounts(countThreshold + localThreshold, localThreshold);
    ASSERT_TLM_Monitor_SIZE(1);
    ASSERT_TLM_Monitor(0, YELLOW);
    ASSERT_from_faultOut_SIZE(0);
    this->clearHistory();

    this->cycleTo(countThreshold + systemThreshold);
    ASSERT_TLM_Monitor_SIZE(1);
    ASSERT_TLM_Monitor(0, RED);
    ASSERT_from_faultOut_SIZE(1);
    ASSERT_from_faultOut(0, COUNTER_HIGH);
}

// ----------------------------------------------------------------------
// Helpers
// ----------------------------------------------------------------------

void MonitoredCounterTester ::dispatchAll() {
    // Bounded: each dispatched message queues at most one state machine signal
    for (FwSizeType i = 0; i < TEST_INSTANCE_QUEUE_DEPTH; i++) {
        if (this->component.m_queue.getMessagesAvailable() == 0) {
            return;
        }
        ASSERT_EQ(this->component.doDispatch(), Fw::QueuedComponentBase::MSG_DISPATCH_OK);
    }
    ASSERT_EQ(this->component.m_queue.getMessagesAvailable(), 0);
}

void MonitoredCounterTester ::cycle(U32 count) {
    for (U32 i = 0; i < count; i++) {
        this->invoke_to_run(0, 0);
        this->dispatchAll();
        this->m_cycles++;
    }
}

void MonitoredCounterTester ::cycleTo(U32 count) {
    ASSERT_GE(count, this->m_cycles);
    this->cycle(count - this->m_cycles);
}

void MonitoredCounterTester ::resetCount() {
    this->clearHistory();
    this->sendCmd_RESET_COUNT(0, 1);
    this->dispatchAll();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, MonitoredCounter::OPCODE_RESET_COUNT, 1, Fw::CmdResponse::OK);
    // Count tracking restarts from the reset
    this->m_cycles = 0;
}

void MonitoredCounterTester ::setMonitoring(const Fw::Enabled& enabled) {
    this->clearHistory();
    this->sendCmd_SET_MONITORING(0, 1, enabled);
    this->dispatchAll();
    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, MonitoredCounter::OPCODE_SET_MONITORING, 1, Fw::CmdResponse::OK);
}

void MonitoredCounterTester ::assertCounts(U32 count, U32 errors) {
    ASSERT_GT(this->tlmHistory_Count->size(), 0);
    ASSERT_EQ(this->tlmHistory_Count->at(this->tlmHistory_Count->size() - 1).arg, count);
    ASSERT_GT(this->tlmHistory_ErrorCount->size(), 0);
    ASSERT_EQ(this->tlmHistory_ErrorCount->at(this->tlmHistory_ErrorCount->size() - 1).arg, errors);
}

}  // namespace Ref
