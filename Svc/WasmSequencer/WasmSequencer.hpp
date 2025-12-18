// ======================================================================
// \title  WasmSequencer.hpp
// \author mstarch
// \brief  hpp file for WasmSequencer component implementation class
// ======================================================================

#ifndef Svc_WasmSequencer_HPP
#define Svc_WasmSequencer_HPP

#include "Svc/WasmSequencer/WasmSequencerComponentAc.hpp"
#include "Os/Condition.hpp"
extern "C"
{
#include "wasm3.h"
}


namespace Svc {

class WasmSequencer final : public WasmSequencerComponentBase {
  public:
    // ----------------------------------------------------------------------
    // Component construction and destruction
    // ----------------------------------------------------------------------

    //! Construct WasmSequencer object
    WasmSequencer(const char* const compName  //!< The component name
    );

    //! Destroy WasmSequencer object
    ~WasmSequencer();

    //! Initialization function
    void init(
          FwSizeType queueDepth, //!< The queue depth
          FwEnumStoreType instance = 0 //!< The instance number
    );

  private:
    // ----------------------------------------------------------------------
    // Handler implementations for typed input ports
    // ----------------------------------------------------------------------

    //! Handler implementation for checkTimers
    //!
    //! port to trigger a wakeup or timeout check. increase frequency
    //! to increase temporal resolution of sequencer
    void checkTimers_handler(FwIndexType portNum,  //!< The port number
                             U32 context           //!< The call order
                             ) override;

    //! Handler implementation for cmdResponseIn
    //!
    //! responses back from commands from the seq
    void cmdResponseIn_handler(FwIndexType portNum,             //!< The port number
                               FwOpcodeType opCode,             //!< Command Op Code
                               U32 cmdSeq,                      //!< Command Sequence
                               const Fw::CmdResponse& response  //!< The command response argument
                               ) override;

    //! Handler implementation for pingIn
    //!
    //! Ping in port
    void pingIn_handler(FwIndexType portNum,  //!< The port number
                        U32 key               //!< Value to return to pinger
                        ) override;

    //! Handler implementation for seqRunIn
    //!
    //! port for requests to run sequences
    void seqRunIn_handler(FwIndexType portNum,            //!< The port number
                          const Fw::StringBase& filename  //!< The sequence file
                          ) override;

    //! Handler implementation for tlmWrite
    //!
    //! port to write all telemetry
    void tlmWrite_handler(FwIndexType portNum,  //!< The port number
                          U32 context           //!< The call order
                          ) override;

  public:
    //! \brief emit a message using the SequenceMessage event
    //!
    void emitMessage(Fw::StringBase& message);

    //! \brief send a command using the SequenceMessage event
    //!
    const Fw::CmdResponse sendCommand(Fw::ComBuffer& command);

    //! \brief get telemetry
    //!
    Fw::TlmValid getTelemetry(FwChanIdType id, Fw::Time& time, Fw::TlmBuffer& val);

    M3Result linkModule();

    void load_sequence(const Fw::StringBase& filename);
    void run_sequence();
  private:
    // ----------------------------------------------------------------------
    // Handler implementations for commands
    // ----------------------------------------------------------------------

    //! Handler implementation for command RUN
    //!
    //! Loads, validates and runs a sequence
    void RUN_cmdHandler(FwOpcodeType opCode,                 //!< The opcode
                        U32 cmdSeq,                          //!< The command sequence number
                        const Fw::CmdStringArg& fileName,    //!< The name of the sequence file
                        Svc::WasmSequencer_BlockState block  //!< Return command status when complete or not
                        ) override;

    //! Handler implementation for command VALIDATE
    //!
    //! Loads and validates a sequence
    void VALIDATE_cmdHandler(FwOpcodeType opCode,              //!< The opcode
                             U32 cmdSeq,                       //!< The command sequence number
                             const Fw::CmdStringArg& fileName  //!< The name of the sequence file
                             ) override;

    //! Handler implementation for command RUN_VALIDATED
    //!
    //! Must be called after VALIDATE. Runs the sequence that was validated.
    void RUN_VALIDATED_cmdHandler(FwOpcodeType opCode,                 //!< The opcode
                                  U32 cmdSeq,                          //!< The command sequence number
                                  Svc::WasmSequencer_BlockState block  //!< Return command status when complete or not
                                  ) override;

    //! Handler implementation for command CANCEL
    //!
    //! Cancels a running or validated sequence. After running CANCEL, the sequencer
    //! should return to IDLE
    void CANCEL_cmdHandler(FwOpcodeType opCode,  //!< The opcode
                           U32 cmdSeq            //!< The command sequence number
                           ) override;
    // For implementing block-on-command
    Os::ConditionVariable m_command_outstanding;
    Os::Mutex m_command_mutex;
    Fw::CmdResponse m_last_command_response;

    bool m_loaded = false;
    U8 m_file_buffer[1024 * 1024];
    IM3Environment m_wasm_environment;
    IM3Runtime m_wasm_runtime;
    IM3Module m_module;

};

}  // namespace Svc

#endif
