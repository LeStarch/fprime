#include "Svc/WasmSequencer/WasmSequencer.hpp"
#include "Fw/Types/String.hpp"

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
const void* emitMessage(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);

//! \brief send a command from WASM code
//!
//! This function is called from WASM code to emit a message via the F Prime event system. It uses the standard WASM3
//! signature for a linked function.
//!
//! This function will load Fw::ComBuffer from memory and convert it to an F Prime command buffer, then send it using
//! the sequencer's sendCommand method.
//!
const void* sendCommand(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);

//! \brief get telemetry from F Prime
//!
//! This function is called from WASM code to emit a message via the F Prime event system. It uses the standard WASM3
//! signature for a linked function.
//!
//! This function will request telemetry from the F Prime telemetry system and return it as a buffer to the WASM3 code.
//!
const void* getTelemetry(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem);