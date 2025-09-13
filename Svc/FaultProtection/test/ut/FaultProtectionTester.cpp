// ======================================================================
// \title  FaultProtection.hpp
// \author mstarch
// \brief  cpp file for FaultProtection test harness implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include "FaultProtectionTester.hpp"
#include <STest/Pick/Pick.hpp>
#include "Svc/FaultProtection/ClearFaultEnumAc.hpp"

#define INSTANCE 0
#define MAX_HISTORY_SIZE 10
#define QUEUE_DEPTH 10

namespace Svc {

// ----------------------------------------------------------------------
// Construction and destruction
// ----------------------------------------------------------------------

Tester ::Tester(void)
    : FaultProtectionGTestBase("Tester", MAX_HISTORY_SIZE), component("FaultProtection"), m_force_error(false) {
    this->initComponents();
    this->connectPorts();
}

Tester ::~Tester(void) {}

// ----------------------------------------------------------------------
// Test Helpers
// ----------------------------------------------------------------------
U32 Tester::invoke_fault(Project::FaultId fault) {
    U32 context = STest::Pick::lowerUpper(0, 0xFFFFFFFF);
    invoke_to_fault(0, fault, context);
    this->component.doDispatch();
    return context;
}

void Tester::test_sequence_helper(const char* response, Project::FaultId fault) {
    U32 context = invoke_fault(fault);

    ASSERT_EVENTS_NoResponse_SIZE(0);
    ASSERT_EVENTS_FaultResponse(0, response, fault, context);
    this->clearHistory();
}

void Tester::test_fatal_helper(Project::FaultId fault) {
    U32 context = invoke_fault(fault);

    ASSERT_EVENTS_FaultResponse_SIZE(0);
    ASSERT_EVENTS_NoResponse(0, fault, context);
    this->clearHistory();
}
void Tester::test_no_response_helper(Project::FaultId fault) {
    U32 context = invoke_fault(fault);

    ASSERT_EVENTS_FaultResponse(0, FaultProtectionComponentImpl::NO_RESPONSE, fault, context);
    ASSERT_EVENTS_NoResponse_SIZE(0);
    this->clearHistory();
}

// ----------------------------------------------------------------------
// Tests
// ----------------------------------------------------------------------
void Tester::test_fatal_response() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        test_fatal_helper(fault);
    }
}

void Tester::test_no_response() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        sendCmd_CLEAR_FAULT_RESPONSE(0, 0, fault, ClearFault::IGNORE_RESPONSE);
        this->component.doDispatch();
        test_no_response_helper(fault);
        sendCmd_CLEAR_FAULT_RESPONSE(0, 0, fault, ClearFault::FATAL_RESPONSE);
        this->component.doDispatch();
        test_fatal_helper(fault);
    }
}

void Tester::test_sequence_response() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        Fw::String sequence(__FILE__);
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence);
        this->component.doDispatch();
        ASSERT_EVENTS_SequenceDoesNotExist_SIZE(0);
        test_sequence_helper(__FILE__, fault);
        sendCmd_CLEAR_FAULT_RESPONSE(0, 0, fault, ClearFault::FATAL_RESPONSE);
        this->component.doDispatch();
        test_fatal_helper(fault);
    }
}

void Tester::test_clear_to_fatal_response() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        Fw::String sequence(__FILE__);
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence);
        this->component.doDispatch();
        ASSERT_EVENTS_SequenceDoesNotExist_SIZE(0);
        sendCmd_CLEAR_FAULT_RESPONSE(0, 0, fault, ClearFault::FATAL_RESPONSE);
        this->component.doDispatch();
        test_fatal_helper(fault);
    }
}

void Tester::test_clear_to_no_response() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        Fw::String sequence(__FILE__);
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence);
        this->component.doDispatch();
        ASSERT_EVENTS_SequenceDoesNotExist_SIZE(0);
        sendCmd_CLEAR_FAULT_RESPONSE(0, 0, fault, ClearFault::IGNORE_RESPONSE);
        this->component.doDispatch();
        test_no_response_helper(fault);
    }
}

void Tester::test_reset_response() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        Fw::String sequence1("/dev/zero");  // Another existent file
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence1);
        this->component.doDispatch();
        ASSERT_EVENTS_SequenceDoesNotExist_SIZE(0);
        Fw::String sequence2(__FILE__);  // Another existent file
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence2);
        this->component.doDispatch();
        test_sequence_helper(__FILE__, fault);
    }
}

void Tester::test_serialized_response() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        Fw::String sequence1("/dev/zero");  // Another existent file
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence1);
        this->component.doDispatch();
        test_sequence_helper("/dev/zero", fault);
        ASSERT_EVENTS_SequenceDoesNotExist_SIZE(0);
        Fw::String sequence2(__FILE__);  // Another existent file
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence2);
        this->component.doDispatch();
        test_sequence_helper(__FILE__, fault);
    }
}

void Tester::test_fatal_on_bad_sequence() {
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        const char* bad_file = "/abc/a123/should-not-exist";
        Project::FaultId fault;
        fault = i;
        Fw::String sequence(bad_file);
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence);
        this->component.doDispatch();
        invoke_fault(fault);
        ASSERT_EVENTS_SequenceDoesNotExist(0, bad_file);
        clearHistory();
        sendCmd_CLEAR_FAULT_RESPONSE(0, 0, fault, ClearFault::FATAL_RESPONSE);
        this->component.doDispatch();
        test_fatal_helper(fault);
    }
}

void Tester::test_fatal_on_failed_sequence() {
    m_force_error = true;
    // Test initial setup is faults
    for (U32 i = 0; i < Project::FaultId::NUM_CONSTANTS; i++) {
        Project::FaultId fault;
        fault = i;
        Fw::String sequence(__FILE__);
        sendCmd_SET_FAULT_RESPONSE(0, 0, fault, sequence);
        this->component.doDispatch();
        invoke_fault(fault);
        ASSERT_EVENTS_SequenceFailed(0, __FILE__);
        clearHistory();
        sendCmd_CLEAR_FAULT_RESPONSE(0, 0, fault, ClearFault::FATAL_RESPONSE);
        this->component.doDispatch();
        test_fatal_helper(fault);
    }
}

// ----------------------------------------------------------------------
// Handlers for typed from ports
// ----------------------------------------------------------------------

void Tester ::from_seqRun_handler(const NATIVE_INT_TYPE portNum, Fw::String& filename) {
    this->pushFromPortEntry_seqRun(filename);
    U32 fault_size = this->eventHistory_FaultResponse->size();
    U32 fatal_size = this->eventHistory_NoResponse->size();
    U32 not_exists = this->eventHistory_SequenceDoesNotExist->size();

    invoke_to_seqDone(0, 0, 0, m_force_error ? Fw::COMMAND_EXECUTION_ERROR : Fw::COMMAND_OK);

    ASSERT_EVENTS_SequenceDoesNotExist_SIZE(not_exists);
    ASSERT_EVENTS_FaultResponse_SIZE(fault_size);
    ASSERT_EVENTS_NoResponse_SIZE(fatal_size);
}

// ----------------------------------------------------------------------
// Helper methods
// ----------------------------------------------------------------------

void Tester ::connectPorts(void) {
    // cmdIn
    this->connect_to_CmdDisp(0, this->component.get_CmdDisp_InputPort(0));

    // seqDone
    this->connect_to_seqDone(0, this->component.get_seqDone_InputPort(0));

    // fault
    this->connect_to_fault(0, this->component.get_fault_InputPort(0));

    // cmdRegOut
    this->component.set_CmdReg_OutputPort(0, this->get_from_CmdReg(0));

    // cmdResponseOut
    this->component.set_CmdStatus_OutputPort(0, this->get_from_CmdStatus(0));

    // logOut
    this->component.set_Log_OutputPort(0, this->get_from_Log(0));

    // LogText
    this->component.set_LogText_OutputPort(0, this->get_from_LogText(0));

    // timeCaller
    this->component.set_Time_OutputPort(0, this->get_from_Time(0));

    // seqRun
    this->component.set_seqRun_OutputPort(0, this->get_from_seqRun(0));
}

void Tester ::initComponents(void) {
    this->init();
    this->component.init(QUEUE_DEPTH, INSTANCE);
}

}  // end namespace Svc
