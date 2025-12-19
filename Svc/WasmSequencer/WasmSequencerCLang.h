#include <stdint.h>
#ifndef WASM_SEQUENCER_CLANG_H
#define WASM_SEQUENCER_CLANG_H
__attribute__((import_module("fprime_core"), import_name("message")))
extern void message_wasm(uint32_t pointer, uint32_t length);

__attribute__((import_module("fprime_core"), import_name("panic")))
extern void panic_wasm(uint32_t pointer, uint32_t length);

__attribute__((import_module("fprime_core"), import_name("exit")))
extern void exit(int32_t status);

__attribute__((import_module("fprime_core"), import_name("command")))
extern int32_t command_wasm(uint32_t pointer, uint32_t length);

__attribute__((import_module("fprime_core"), import_name("telemetry")))
extern int32_t telemetry_wasm(uint32_t telemetryId, uint32_t time_out, uint32_t time_length, uint32_t value_out, uint32_t value_length);

__attribute__((import_module("fprime_core"), import_name("rsleep")))
extern void rsleep_wasm(uint64_t microseconds);


const int32_t MAX_STRING_LENGTH = 256; // From: FW_FIXED_LENGTH_STRING_SIZE

//! \brief Get the length of a string (bounded by MAX_STRING_LENGTH)
//!
//! This function will return the length of a string, but will not exceed MAX_STRING_LENGTH.
//!
//! \param str bounded string to get the length of
//! \return length of the string
uint32_t bounded_string_length(const char* str) {
    uint32_t length = 0;
    for (length = 0; length < MAX_STRING_LENGTH; length++) {
        if (str[length] == '\0') {
            break;
        }
    }
    return length;
}

//! \brief Send a message to the sequencer
//!
//! This function will send a message to the sequencer, which will be displayed as an event.
//!
//! \param message message to send to the sequencer
void message(const char* message) {
    uint32_t pointer = (uint32_t)(message);
    uint32_t length = bounded_string_length(message);
    message_wasm(pointer, length);
}
//! \brief Panic with a message
//!
//! This function will send a panic message to the sequencer, which will cause the sequence to cease.
//!
//! \param message message to send to the sequencer
void panic(const char* message) {
    uint32_t pointer = (uint32_t)(message);
    uint32_t length = bounded_string_length(message);
    panic_wasm(pointer, length);
}

//! \brief command the host with a command
//!
//! This function will send a command to the host, which will be executed by the host.
//!
//! \param opcode command opcode to send to the host
//! \param command command buffer to fill with the command data
//! \param length length of the command buffer
int32_t command(int32_t opcode, uint8_t* args, int32_t length) {
    uint8_t command_buffer[2 + 4 + length];
    command_buffer[0] = 0; // 4 bytes for the opcode, 4 bytes for the length
    command_buffer[1] = 0;
    command_buffer[2] = (opcode >> 24) & 0xFF;
    command_buffer[3] = (opcode >> 16) & 0xFF;
    command_buffer[4] = (opcode >> 8) & 0xFF;
    command_buffer[5] = opcode & 0xFF;
    for (int32_t i = 0; i < length; i++) {
        command_buffer[6 + i] = args[i];
    }

    uint32_t command_pointer = (uint32_t)(command_buffer);
    int32_t response = command_wasm(command_pointer, sizeof(command_buffer));
    return response;
}
//! \brief Get telemetry from the host
//!
//! This function will get telemetry data from the host.
//!
//! \param id telemetry telemetry ID
//! \param time telemetry time buffer
//! \param time_length length of the telemetry time buffer
//! \param value telemetry value buffer
//! \param value_length length of the telemetry value buffer
int32_t telemetry(uint32_t id, const uint8_t* time, uint32_t time_length, const uint8_t* value, uint32_t value_length) {
    uint32_t time_pointer = (uint32_t)(time);
    uint32_t value_pointer = (uint32_t)(value);
    int32_t response = telemetry_wasm(id, time_pointer, time_length, value_pointer, value_length);
    return response;
}

//! \brief Sleep for a given number of microseconds
//!
//! This function will sleep for the given number of microseconds relative to the current time.
//! \param microseconds number of microseconds to sleep
void rsleep(uint64_t microseconds) {
    rsleep_wasm(microseconds);
}


#endif // WASM_SEQUENCER_CLANG_H