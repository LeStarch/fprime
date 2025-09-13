// ======================================================================
// \title  FaultProtection.cpp
// \author mstarch
// \brief  cpp file for FaultProtection component implementation class
//
// \copyright
// Copyright 2009-2015, by the California Institute of Technology.
// ALL RIGHTS RESERVED.  United States Government Sponsorship
// acknowledged.
//
// ======================================================================

#include <Svc/FaultProtection/FaultProtection.hpp>
#include "Fw/FPrimeBasicTypes.hpp"
#include "Os/File.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Construction, initialization, and destruction
// ----------------------------------------------------------------------
static_assert(FaultCfg::FaultId::NUM_CONSTANTS > 1, "No faults defined");

FaultProtection ::FaultProtection(const char* const compName)
    : FaultProtectionComponentBase(compName), m_current_sequence(m_response_table[0]), m_running(false) {
    for (U32 i = 0; i < FW_NUM_ARRAY_ELEMENTS(m_response_table); i++) {
        m_response_table[i] = NO_SEQUENCE;
    }
}

FaultProtection ::~FaultProtection(void) {}

// ----------------------------------------------------------------------
// Handler implementations for user-defined typed input ports
// ----------------------------------------------------------------------

void FaultProtection ::seqDone_handler(const FwIndexType portNum,
                                       FwOpcodeType opCode,
                                       U32 cmdSeq,
                                       const Fw::CmdResponse& response) {
    // Not running anymore
    m_lock.lock();
    m_running = false;
    m_lock.unLock();

    // Handle a sequence failure
    if (response != Fw::CmdResponse::OK) {
        Fw::LogStringArg arg = m_current_sequence;
        log_FATAL_SequenceFailed(arg);
    }
}

void FaultProtection ::fault_handler(const FwIndexType portNum, const FaultCfg::FaultId& identifier, U32 context) {
    Fw::String& current_string = m_response_table[fault_to_index(identifier)];
    Fw::LogStringArg arg = current_string;

    // No sequence found so this will result in a FATAL
    if (current_string == NO_SEQUENCE) {
        log_FATAL_NoResponse(identifier, context);
    }
    // Response specifically disabled. Only the response EVR sent
    else if (current_string == NO_RESPONSE) {
        log_ACTIVITY_HI_FaultResponse(arg, identifier, context);
    }
    // Response that calls sequence engine
    else {
        m_current_sequence = current_string;
        log_ACTIVITY_HI_FaultResponse(arg, identifier, context);
        // Starting sequence anymore
        m_lock.lock();
        m_running = true;
        m_lock.unLock();
        seqRun_out(0, current_string);

        // Block this component until it is done
        bool loop = false;
        do {
            Os::Task::delay(Fw::TimeInterval(1, 0));  // No busy wait
            m_lock.lock();
            loop = m_running;
            m_lock.unLock();
        } while (loop);
    }
}

U32 FaultProtection ::fault_to_index(FaultCfg::FaultId fault) {
    const U32 index = static_cast<U32>(fault.e);
    FW_ASSERT(index < FW_NUM_ARRAY_ELEMENTS(m_response_table), static_cast<FwAssertArgType>(index));
    return index;
}

// ----------------------------------------------------------------------
// Command handler implementations
// ----------------------------------------------------------------------

void FaultProtection ::SET_FAULT_RESPONSE_cmdHandler(const FwOpcodeType opCode,
                                                     const U32 cmdSeq,
                                                     FaultCfg::FaultId faultId,
                                                     const Fw::CmdStringArg& sequencePath) {
    // Make sure the file can be opened for read
    Os::File sequence_file;
    Os::File::Status status = sequence_file.open(sequencePath.toChar(), Os::File::OPEN_READ);
    sequence_file.close();
    // Successful file opens register sequences
    if (status == Os::File::OP_OK) {
        m_response_table[fault_to_index(faultId)] = sequencePath;
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
    // Unsuccessful statues print warning and set FATAL response
    else {
        Fw::LogStringArg arg = sequencePath;
        log_WARNING_HI_SequenceDoesNotExist(arg);
        m_response_table[fault_to_index(faultId)] = FaultProtection::NO_SEQUENCE;
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    }
}

void FaultProtection ::CLEAR_FAULT_RESPONSE_cmdHandler(const FwOpcodeType opCode,
                                                       const U32 cmdSeq,
                                                       FaultCfg::FaultId faultId,
                                                       Svc::ClearFault newResponse) {
    m_response_table[fault_to_index(faultId)] =
        (newResponse == Svc::ClearFault::IGNORE_RESPONSE) ? FaultProtection::NO_RESPONSE : FaultProtection::NO_SEQUENCE;
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // end namespace Svc
