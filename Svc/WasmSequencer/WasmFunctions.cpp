#include "Svc/WasmSequencer/WasmSequencer.hpp"
#include "Svc/WasmSequencer/WasmFunctions.hpp"
#include "Fw/Types/String.hpp"
#include "Fw/Com/ComBuffer.hpp"
#include "Fw/Com/ComPacket.hpp"
#include "Fw/Types/Serializable.hpp"
#include "Fw/Logger/Logger.hpp"

const void* emitMessage(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) { 
    //m3ApiReturnType(void); // Return type is void
    m3ApiGetArg(I32, stringOffset); // First argument is the string pointer as a WASM-compatible I32
    m3ApiGetArg(I32, stringLength); // Second argument is the string length as a WASM-compatible I32
    // Copy the string data from WASM memeory into an Fw::String
    Fw::String argument;
    // TODO: fix this stupid copy
    char buffer[2] = {'\0', '\0'};
    for (I32 i = 0; i < stringLength; i++) {
        buffer[0] = *(reinterpret_cast<char*>(mem) + stringOffset + i);
        argument += buffer;
    }
    Svc::WasmSequencer* sequencer = reinterpret_cast<Svc::WasmSequencer*>(_ctx->userdata);
    FW_ASSERT(sequencer != nullptr);
    sequencer->emitMessage(argument);
    //m3ApiReturn(nullptr); // No return value
    m3ApiSuccess()
}

const void* sendCommand(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) { 
    m3ApiReturnType(int32_t); // Return type is void
    m3ApiGetArg(I32, opcode); // First argument is the opcode
    m3ApiGetArg(I32, dataPointer); // Second argument is the data pointer as a WASM-compatible I32
    m3ApiGetArg(I32, dataLength); // Third argument is the data length as a WASM-compatible I32
    // Copy the string data from WASM memory into an Fw::String
    Fw::ComBuffer argument;
    argument.serializeFrom(static_cast<FwPacketDescriptorType>(ComCfg::Apid::T::FW_PACKET_COMMAND));
    argument.serializeFrom(static_cast<FwOpcodeType>(opcode));
    argument.serializeFrom(static_cast<U8*>(mem) + dataPointer, static_cast<FwSizeType>(dataLength), Fw::Serialization::OMIT_LENGTH);
    Svc::WasmSequencer* sequencer = reinterpret_cast<Svc::WasmSequencer*>(_ctx->userdata);
    FW_ASSERT(sequencer != nullptr);
    Fw::CmdResponse response = sequencer->sendCommand(argument); // Will block (non-busy) until the command is done
    int32_t responseCode = static_cast<int32_t>(response.e);
    m3ApiReturn(responseCode); // No return value
    m3ApiSuccess()
}

const void* getTelemetry(IM3Runtime rt, IM3ImportContext _ctx, uint64_t* _sp, void* mem) {
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
    U32 value;
    Fw::TlmBuffer dataMemory;
    Fw::TlmValid valid = sequencer->getTelemetry(static_cast<FwChanIdType>(id), timeMemory, dataMemory);
    dataMemory.deserializeTo(value);
    dataMemory.resetDeser();
    Fw::Logger::log("Telemetry value: %" PRI_U32 "\n", value);

    Fw::ExternalSerializeBuffer timeMemoryWrapper(static_cast<U8*>(mem) + timeAddress, static_cast<FwSizeType>(timeLength));
    Fw::ExternalSerializeBuffer valueMemoryWrapper(static_cast<U8*>(mem) + dataAddress, static_cast<FwSizeType>(dataLength));
    FW_ASSERT(timeMemoryWrapper.serializeFrom(timeMemory) == Fw::SerializeStatus::FW_SERIALIZE_OK);
    FW_ASSERT(valueMemoryWrapper.serializeFrom(dataMemory) == Fw::SerializeStatus::FW_SERIALIZE_OK);
    m3ApiReturn(valid.e);
}