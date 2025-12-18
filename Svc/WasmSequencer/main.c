#include "WasmSequencerCLang.h"

int32_t processCommandErrored(unsigned char * buffer, char * string) {
    int32_t value = buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    int32_t return_value = value;
    for (int i = 0; i < 8; i++) {
        string[9 - i] = (value & 0xFF) + '0';
        value >>= 8;
    }
    return return_value;
}

void memHexify(unsigned char* mem, char * buffer, int32_t length) {
    for (unsigned int i = 0; i < length; i++) {
        buffer[i * 2] = "0123456789ABCDEF"[(mem[i] >> 4) & 0x0F];
        buffer[i * 2 + 1] = "0123456789ABCDEF"[mem[i] & 0x0F];
        buffer[i * 2 + 2] = ' ';
    }
    buffer[length * 3] = '\0';
}

int main() {
    int op = 16777217;
    unsigned char timeBuffer[100];
    unsigned char dataBuffer[536];
    char value[10] = {'0', 'x', '0', '0', '0', '0', '0', '0', '0', '0'};

    const int dumpLen = 20;
    char valueBuffer[20 * 3 + 1];


    int32_t status = getTelemetry(16777217, timeBuffer, sizeof(timeBuffer), dataBuffer, sizeof(dataBuffer));
    memHexify(dataBuffer, valueBuffer, dumpLen);
    sequenceMessage(valueBuffer);
    if (status != 0) {
        sequenceMessage("Failed to get initial telemetry");
        return 1; 
    }
    int32_t commandsErrored = processCommandErrored(dataBuffer + 2, value);
    if (commandsErrored != 0) {
        sequenceMessage("Error count is not zero");
        sequenceMessage(value);
    }

    for (int i = 0; i < 2; i++) {
        sequenceMessage("Hello from the void!");
        status = sendCommand(op, 0, 0); // NO_OP has no arguments
        if (status != 0) {
            sequenceMessage("Failed to send command");
            op--;       
        }
    }
    status = getTelemetry(16777217, timeBuffer, sizeof(timeBuffer), dataBuffer, sizeof(dataBuffer));
    if (status != 0) {
        sequenceMessage("Failed to get telemetry -- again");
    }
    else if (commandsErrored != processCommandErrored(dataBuffer + 2, value)) {
        sequenceMessage("Error count changed!!!!");
        sequenceMessage(value);
    } 
    return 20; 
}
