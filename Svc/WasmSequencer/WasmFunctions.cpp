#include "Svc/WasmSequencer/WasmSequencer.hpp"
#include "Svc/WasmSequencer/WasmFunctions.hpp"
#include "Fw/Types/String.hpp"
#include "Fw/Com/ComBuffer.hpp"
#include "Fw/Com/ComPacket.hpp"
#include "Fw/Types/Serializable.hpp"

//! \brief Fill an Fw::String with the given string data from WASM memory
//!
//! Since Fw::String does not provide a bounded copy, there is no clean way to copy a string into it without using an
//! intermediate buffer.
//!
//! \param string The Fw::String to fill
//! \param string_base The base pointer to the string data in WASM memory
//! \param length The length of the string data in bytes
void fill_string(Fw::String& string, const char* string_base, uint32_t length) {
    // Copy the string data from WASM memory into an Fw::String
    char buffer[2] = {'\0', '\0'};

    // Copy each character into the Fw::String one at a time using the string copy function
    for (uint32_t i = 0; i < length; i++) {
        buffer[0] = *(string_base + i);
        buffer[1] = '\0'; // Re-enforce null termination
        string += buffer;
    }
}


const void* message_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) { 
    m3ApiGetArg(U32, stringOffset); // First argument is the string pointer as a WASM-compatible I32
    m3ApiGetArg(U32, stringLength); // Second argument is the string length as a WASM-compatible I32

    Svc::WasmSequencer* sequencer = reinterpret_cast<Svc::WasmSequencer*>(_ctx->userdata);
    FW_ASSERT(sequencer != nullptr);
    Fw::String argument;
    fill_string(argument, static_cast<const char*>(mem) + stringOffset, stringLength);
    sequencer->message_sequencer(argument);
    //m3ApiReturn(nullptr); // No return value
    m3ApiSuccess()
}

const void* panic_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) { 
    m3ApiGetArg(U32, stringOffset); // First argument is the string pointer as a WASM-compatible I32
    m3ApiGetArg(U32, stringLength); // Second argument is the string length as a WASM-compatible I32

    Svc::WasmSequencer* sequencer = reinterpret_cast<Svc::WasmSequencer*>(_ctx->userdata);
    FW_ASSERT(sequencer != nullptr);
    Fw::String argument;
    fill_string(argument, static_cast<const char*>(mem) + stringOffset, stringLength);
    sequencer->panic_sequencer(argument);
    m3ApiTrap("Panic halted execution");
}

const void* exit_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) { 
    m3ApiGetArg(I32, return_code); // First argument is the return code as a WASM-compatible I32
    Svc::WasmSequencer* sequencer = reinterpret_cast<Svc::WasmSequencer*>(_ctx->userdata);
    FW_ASSERT(sequencer != nullptr);
    sequencer->exit_sequencer(return_code);
    m3ApiTrap("Exit handled");
}

const void* command_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) { 
    m3ApiReturnType(int32_t); // Return type is void
    m3ApiGetArg(I32, dataPointer); // Second argument is the data pointer as a WASM-compatible I32
    m3ApiGetArg(I32, dataLength); // Third argument is the data length as a WASM-compatible I32
    // Copy the string data from WASM memory into an Fw::String
    Fw::ComBuffer argument;
    argument.serializeFrom(static_cast<U8*>(mem) + dataPointer, static_cast<FwSizeType>(dataLength), Fw::Serialization::OMIT_LENGTH);
    Svc::WasmSequencer* sequencer = reinterpret_cast<Svc::WasmSequencer*>(_ctx->userdata);
    FW_ASSERT(sequencer != nullptr);
    Fw::CmdResponse response = sequencer->command_sequence(argument); // Will block (non-busy) until the command is done
    int32_t responseCode = static_cast<int32_t>(response.e);
    m3ApiReturn(responseCode); // No return value
    m3ApiSuccess()
}



const void* telemetry_host(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) {
    m3ApiReturnType(int32_t); // Return type is void
    m3ApiGetArg(I32, id);
    m3ApiGetArg(I32, timeAddress);
    m3ApiGetArg(I32, timeLength);
    m3ApiGetArg(I32, dataAddress);
    m3ApiGetArg(I32, dataLength);
    
    Svc::WasmSequencer* sequencer = reinterpret_cast<Svc::WasmSequencer*>(_ctx->userdata);
    FW_ASSERT(sequencer != nullptr);
    FW_ASSERT(static_cast<size_t>(timeLength) >= Fw::Time::SERIALIZED_SIZE);
    FW_ASSERT(static_cast<size_t>(dataLength) >= Fw::TlmBuffer::SERIALIZED_SIZE);
    Fw::Time timeMemory;
    Fw::TlmBuffer dataMemory;
    Fw::TlmValid valid = sequencer->telemetry_sequence(static_cast<FwChanIdType>(id), timeMemory, dataMemory);

    Fw::ExternalSerializeBuffer timeMemoryWrapper(static_cast<U8*>(mem) + timeAddress, static_cast<FwSizeType>(timeLength));
    Fw::ExternalSerializeBuffer valueMemoryWrapper(static_cast<U8*>(mem) + dataAddress, static_cast<FwSizeType>(dataLength));
    FW_ASSERT(timeMemoryWrapper.serializeFrom(timeMemory) == Fw::SerializeStatus::FW_SERIALIZE_OK);
    FW_ASSERT(valueMemoryWrapper.serializeFrom(dataMemory) == Fw::SerializeStatus::FW_SERIALIZE_OK);
    m3ApiReturn(valid.e);
}