// ======================================================================
// \title  WasmSequencer.cpp
// \author mstarch
// \brief  cpp file for WasmSequencer component implementation class
// ======================================================================

#include "Svc/WasmSequencer/WasmSequencer.hpp"
#include "Os/File.hpp"
#include "Fw/Logger/Logger.hpp"

#include "Svc/WasmSequencer/WasmFunctions.hpp"

namespace Svc {

// ----------------------------------------------------------------------
// Component construction and destruction
// ----------------------------------------------------------------------

WasmSequencer ::WasmSequencer(const char* const compName) : WasmSequencerComponentBase(compName),
    m_wasm_environment(nullptr), m_wasm_runtime(nullptr) {}

WasmSequencer ::~WasmSequencer() {}

void WasmSequencer::init(
        FwSizeType queueDepth, //!< The queue depth
        FwEnumStoreType instance //!< The instance number
) {
    WasmSequencerComponentBase::init(queueDepth, instance);
    m_wasm_environment = m3_NewEnvironment();
    FW_ASSERT(m_wasm_environment != nullptr);
    // Create a new runtime with 1MB of memory
    m_wasm_runtime = m3_NewRuntime(m_wasm_environment, 1024 * 1024, NULL);
    FW_ASSERT(m_wasm_runtime != nullptr);
}


// ----------------------------------------------------------------------
// Handler implementations for typed input ports
// ----------------------------------------------------------------------

void WasmSequencer ::checkTimers_handler(FwIndexType portNum, U32 context) {
    // TODO
}

void WasmSequencer ::cmdResponseIn_handler(FwIndexType portNum,
                                           FwOpcodeType opCode,
                                           U32 cmdSeq,
                                           const Fw::CmdResponse& response) {
    // Update the last command response and notify the waiting thread
    {
        Os::ScopeLock lock(this->m_command_mutex);
        this->m_last_command_response = response;
    }
    this->m_command_outstanding.notify();
}

void WasmSequencer ::pingIn_handler(FwIndexType portNum, U32 key) {
    this->pingOut_out(portNum, key);
}

void WasmSequencer ::seqRunIn_handler(FwIndexType portNum, const Fw::StringBase& filename) {
    // TODO
}

void WasmSequencer ::tlmWrite_handler(FwIndexType portNum, U32 context) {
    // TODO
}

// ----------------------------------------------------------------------
// Handler implementations for commands
// ----------------------------------------------------------------------

M3Result WasmSequencer ::linkFunctions() {
    M3Result result = m3Err_none;
    for (FwSizeType i = 0; i < FW_NUM_ARRAY_ELEMENTS(WASM_LINK_FUNCTIONS); i++) {
        result = m3_LinkRawFunctionEx(this->m_module, WASM_MODULE_NAME, WASM_LINK_FUNCTIONS[i].name,
            WASM_LINK_FUNCTIONS[i].signature, WASM_LINK_FUNCTIONS[i].function, this
        );
        // Log a warning if we fail to link a function, but continue linking the rest to flush out other errors
        if (result != m3Err_none) {
            Fw::Logger::log("[ERROR] Failed to link wasm function: %s\n", result);
        }
    }
    return result;
}

void WasmSequencer ::message_sequencer(Fw::StringBase& message) {
    this->log_ACTIVITY_HI_SequenceMessage(message);
}

void WasmSequencer ::panic_sequencer(Fw::StringBase& message) {
    this->log_WARNING_HI_SequencePanic(message);
}

void WasmSequencer ::exit_sequencer(int32_t exit_code) {
    if (exit_code != 0) {
        this->log_WARNING_HI_SequenceFailed(exit_code);
    } else {
        this->log_ACTIVITY_HI_SequenceSucceeded();
    }
}

void WasmSequencer ::rsleep_sequencer(Fw::TimeInterval& sleep_interval) {
    // Sleep for the given interval
    Os::Task::delay(sleep_interval);
}


const Fw::CmdResponse  WasmSequencer ::command_sequence(Fw::ComBuffer& command) {
    this->m_command_mutex.lock();
    this->cmdOut_out(0, command, 0);
    this->m_command_outstanding.wait(this->m_command_mutex);
    Fw::CmdResponse response = this->m_last_command_response;
    this->m_command_mutex.unlock();
    return response;
}

Fw::TlmValid  WasmSequencer ::telemetry_sequence(FwChanIdType id, Fw::Time& time, Fw::TlmBuffer& val) {
    return this->getTlmChan_out(0, id, time, val);
}

void WasmSequencer ::load_sequence(const Fw::StringBase& fileName) {
    m_loaded = false;
    Os::File file;
    Os::File::Status status = file.open(fileName.toChar(), Os::File::OPEN_READ);
    FwSizeType readSize = sizeof(m_file_buffer);
    if (status == Os::File::OP_OK) {
        status = file.read(m_file_buffer, readSize);
    }
    if (status == Os::File::OP_OK && readSize < std::numeric_limits<uint32_t>::max()) {
        M3Result result = m3_ParseModule(m_wasm_environment, &m_module, m_file_buffer, static_cast<uint32_t>(readSize));
        if (result == m3Err_none) {
            result = m3_LoadModule(m_wasm_runtime, m_module);
        }
        if (result == m3Err_none) {
            result = this->linkFunctions();
        }
        if (result == m3Err_none) {
            IM3Function function;
            result = m3_FindFunction (&function, m_wasm_runtime, "main");
        }
        if (result == m3Err_none) {
            m_loaded = true;
        } else {
            Fw::Logger::log("[ERROR] Failed to load wasm sequence: %s\n", result);
        }
    } else if (status != Os::File::OP_OK) {
        Fw::Logger::log("[ERROR] Failed to read wasm sequence file: %d\n", status);
    } else if (readSize >= std::numeric_limits<uint32_t>::max()) {
        Fw::Logger::log("[ERROR] Wasm sequence file too large: %zu\n", readSize);
    }
}

void WasmSequencer ::run_sequence() {
    if (!m_loaded) {
        return;
    }
    int output = 0;
    IM3Function function;
    M3Result result = m3_FindFunction (&function, m_wasm_runtime, "main");
    if (result == m3Err_none) {
        const char* args[2] = {"something\0", "3\0"};
        result = m3_CallV(function, 2, args);
    } else {
        Fw::Logger::log("[ERROR] Failed to find wasm main function: %s\n", result);
    }
    if (result == m3Err_none) {
        result = m3_GetResultsV(function, &output);
    } else {
        Fw::Logger::log("[ERROR] Failed to call wasm main function: %s\n", result);
    }
    if (result != m3Err_none) {
        Fw::Logger::log("[ERROR] Failed to get results wasm: %s\n", result);
    } else {
        Fw::Logger::log("[INFO] Wasm sequence completed with output: %d\n", output);
    }
}

void WasmSequencer ::RUN_cmdHandler(FwOpcodeType opCode,
                                    U32 cmdSeq,
                                    const Fw::CmdStringArg& fileName,
                                    Svc::WasmSequencer_BlockState block) {
    this->load_sequence(fileName);
    if (!m_loaded) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    } else {
        if (block == Svc::WasmSequencer_BlockState::NO_BLOCK) {
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        }
        this->run_sequence();
        if (block == Svc::WasmSequencer_BlockState::BLOCK) {
            this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
        }
    }
}

void WasmSequencer ::VALIDATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, const Fw::CmdStringArg& fileName) {
    this->load_sequence(fileName);
    if (!m_loaded) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
    } else {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}

void WasmSequencer ::RUN_VALIDATED_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Svc::WasmSequencer_BlockState block) {
    if (!m_loaded) {
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::EXECUTION_ERROR);
        return;
    } else {
        this->run_sequence();
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
    }
}

void WasmSequencer ::CANCEL_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // TODO
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

}  // namespace Svc
