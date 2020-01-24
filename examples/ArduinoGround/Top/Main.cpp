#include <examples/ArduinoGpsTracker/Top/Components.hpp>
#include <Fw/Types/Generic/GenericLogAssert.hpp>
#include <Os/Log.hpp>
#ifdef ARDUINO
    #include <Os/Arduino/StreamLog.hpp>
    #include <Arduino.h>
#endif
Fw::LogAssertHook assert;
/**
 * Main function.
 */
int main(int argc, char* argv[]) {
	assert.registerHook();
#ifdef ARDUINO
    Serial1.begin(115200);
    Os::setArduinoStreamLogHandler(&Serial1);
#endif
    Os::Log::logMsg("[SETUP] Constructing system\n", 0, 0, 0, 0, 0, 0);
    constructApp();
    return 0;
}
