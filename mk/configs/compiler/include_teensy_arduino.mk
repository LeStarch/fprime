#####
#  Teensy/Arduino Include File
#
#  Sets up the necessary paths and includes for compiling with Teensyduino (Arduino with teensy installed)
#  as is recommended from the Teensy vendor.
#
#  Author: M Starch (lestarch@googlemail.com)
#
#####

TEENSY=?teensy32
ifneq ($(TEENSY),teensy)
  TEENSY_DIR=teensy3
else
  TEENSY_DIR=teensy
endif

ifeq ($(ARDUINOPATH),)
  $(error Required ARUDINOPATH not set)
endif
ARDUINO_BUILD=TRUE
#Set the directory of the Arduino implementation directory
ARDUINOIMPL=$(ARDUINOPATH)/hardware/teensy/avr/cores/$(TEENSY_DIR)
ARDUINOLIB=$(ARDUINOPATH)/hardware/teensy/avr/libraries

#Options needed by many Arduino libraries to configure for Teensy 3.x
OPTIONS += -D__$(MCU)__ -DARDUINO=10805 -DTEENSYDUINO=141

#Path to Arduino's Teensy compiler
COMPILERPATH = $(abspath $(ARDUINOPATH)/hardware/tools/arm/bin)

#Setup objects
ARDUINO_OBJS := $(wildcard $(ARDUINOIMPL)/*.o)

#Sets up includes for the Teensy implementation of the Arduino framework 
ARDUINO_INCLUDES = -I$(ARDUINOIMPL) -I$(BUILD_ROOT)/Fw/Types/Arduino

#TODO: add library includes here, per the example below
ARDUINO_INCLUDES += -I$(ARDUINOLIB)/SPI


