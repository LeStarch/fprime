#include <stdint.h>
__attribute__((import_module("fprime"), import_name("sequenceMessage")))
extern void sequenceMessage_host(int32_t ptr, int32_t length);

__attribute__((import_module("fprime"), import_name("sendCommand")))
extern int32_t sendCommand_host(int32_t opcode, int32_t arg_pointer, int32_t arg_length);

__attribute__((import_module("fprime"), import_name("sendCommand")))
extern int32_t sendCommand_host(int32_t opcode, int32_t arg_pointer, int32_t arg_length);

__attribute__((import_module("fprime"), import_name("getTelemetry")))
extern int32_t getTelemetry_host(int32_t id, int32_t time_pointer, int32_t time_length, int32_t value_pointer, int32_t value_length);

const int32_t MAX_STRING_LENGTH = 2048;

void sequenceMessage(const char* message) {
    int32_t pointer = (int32_t)(message);
    int32_t length = 0;
    for (length = 0; length < MAX_STRING_LENGTH; length++) {
        if (message[length] == '\0') {
            break;
        }
    }
    sequenceMessage_host(pointer, length);
}

int32_t sendCommand(int32_t opcode, const uint8_t* argument, int32_t arg_length) {
    int32_t arg_pointer = (int32_t)(argument);
    int32_t response = sendCommand_host(opcode, arg_pointer, arg_length);
    return response;
}

int32_t getTelemetry(int32_t id, const uint8_t* time, int32_t time_length, const uint8_t* value, int32_t value_length) {
    int32_t time_pointer = (int32_t)(time);
    int32_t value_pointer = (int32_t)(value);
    int32_t response = getTelemetry_host(id, time_pointer, time_length, value_pointer, value_length);
    return response;
}
