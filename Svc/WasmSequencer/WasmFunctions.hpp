#include "Svc/WasmSequencer/WasmSequencer.hpp"
#include "Fw/Types/String.hpp"

#ifndef SVC_WASM_SEQUENCER_HPP
#define SVC_WASM_SEQUENCER_HPP
//! \brief emit a message from WASM code
//!
//! This function is called from WASM code to emit a message via the F Prime event system. It uses the standard WASM3
//! signature for a linked function.
//!
//! This function will load a char* from memory and convert it to an F Prime string, then emit it using the sequencer's
//! emitMessage method.
//!
//! \param rt the WASM3 runtime instance
//! \param _ctx the WASM3 import context. Contains a pointer to the sequencer instance.
//! \param _sp the WASM3 stack pointer
//! \param mem the WASM3 memory
//! \return nullptr on success, or a pointer to an error message on failure
const void* message_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);

//! \brief emit a message and stop the sequencer
//!
//! This function is called from WASM code to emit a message via the F Prime event system. It uses the standard WASM3
//! signature for a linked function.
//!
//! This function will load a char* from memory and convert it to an F Prime string, then emit it using the sequencer's
//! emitMessage method. It will cancel the running sequence.
//!
//! \param rt the WASM3 runtime instance
//! \param _ctx the WASM3 import context. Contains a pointer to the sequencer instance.
//! \param _sp the WASM3 stack pointer
//! \param mem the WASM3 memory
//! \return nullptr on success, or a pointer to an error message on failure
const void* panic_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);

//! \brief send a command from WASM code
//!
//! This function is called from WASM code to emit a message via the F Prime event system. It uses the standard WASM3
//! signature for a linked function.
//!
//! This function will load Fw::ComBuffer from memory and convert it to an F Prime command buffer, then send it using
//! the sequencer's sendCommand method.
//!
const void* command_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);

//! \brief exit from a sequence from WASM code
//!
//! This function is called from WASM code to emit a message via the F Prime event system. It uses the standard WASM3
//! signature for a linked function.
//!
//! This function will exit the current sequence with the supplied return code.
const void* exit_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);


//! \brief get telemetry from F Prime
//!
//! This function is called from WASM code to emit a message via the F Prime event system. It uses the standard WASM3
//! signature for a linked function.
//!
//! This function will request telemetry from the F Prime telemetry system and return it as a buffer to the WASM3 code.
//!
const void* telemetry_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);

//! \brief sleep for a given number of microseconds relative to the current time
//!
//! This function is called from WASM code to sleep for a given number of microseconds. It uses the standard WASM3
//! signature for a linked function.
//! This function will call the sequencer's sleep method.
const void* rsleep_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);

// Module name for F Prime/WASM3
static constexpr const char* WASM_MODULE_NAME = "fprime_core";

//! \brief Shape of a function binding in WASM3
struct WasmLinking {
    const char* name; //!< Name of the function in WASM3
    const char* signature; //!< Signature of the function in WASM3 (in shorthand form)
    const M3RawCall function; //!< Pointer to the function in C++ that will be called
};

static constexpr WasmLinking WASM_LINK_FUNCTIONS[] = {
    {"message", "v(ii)", &message_host},
    {"panic",   "v(ii)", &panic_host},
    {"exit",   "v(i)", &exit_host},
    {"command", "i(ii)", &command_host},
    {"telemetry", "i(iiiii)", &telemetry_host},
    {"rsleep", "v(I)", &rsleep_host}
};

#endif // SVC_WASM_SEQUENCER_HPP