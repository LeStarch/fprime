SRC = RadioWrapperComponentAi.xml \
      RadioWrapper.cpp

HDR = RadioWrapper.hpp	

SRC_TEENSY32 = RadioWrapperArduino.cpp \
	       ../RFM69/RFM69.cpp

COMPARGS = -I$(BUILD_ROOT)/examples/Arduino/RFM69
