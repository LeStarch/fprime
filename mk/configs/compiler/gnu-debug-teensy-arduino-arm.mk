#####
#  Teensy/Arduino Include File
#
#  Sets up the necessary paths and includes for compiling with Teensyduino (Arduino with teensy installed)
#  as is recommended from the Teensy vendor.
#
#  Author: M Starch (lestarch@googlemail.com)
#
#####

TEENSY ?= teensy32
ifeq ($(TEENSY),teensy32)
    MCU = MK20DX256
    FHZ = 72000000
    CPUARCH = cortex-m4    
else
    #TODO: need to implement more boards
    $(error Teensy variant $(TEENSY) not supported)
endif

include $(BUILD_ROOT)/mk/configs/compiler/include_common.mk
include $(BUILD_ROOT)/mk/configs/compiler/gnu-common.mk
include $(BUILD_ROOT)/mk/configs/compiler/defines_common.mk
include $(BUILD_ROOT)/mk/configs/compiler/include_teensy_arduino.mk

# set your MCU type here, or make command line `make MCU=MK20DX256`

# make it lower case
LOWER_MCU := $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$(MCU)))))))))))))))))))))))))))
MCU_LD = $(ARDUINOPATH)/hardware/teensy/avr/cores/teensy3/$(LOWER_MCU).ld

#Teensy configurable options
OPTIONS += -DF_CPU=72000000 -DUSB_SERIAL -DLAYOUT_US_ENGLISH -DUSING_MAKEFILE

#Update compiler path, if not set
COMPILERPATH ?= /usr/bin

# names for the compiler programs
CC = $(COMPILERPATH)/arm-none-eabi-gcc
CXX = $(COMPILERPATH)/arm-none-eabi-g++
GCOV = $(COMPILERPATH)/arm-none-eabi-gcov
GCOV = $(COMPILERPATH)/arm-none-eabi-ar
OBJCOPY = $(COMPILERPATH)/arm-none-eabi-objcopy
SIZE = $(COMPILERPATH)/arm-none-eabi-size

CC_MUNCH := $(BUILD_ROOT)/mk/bin/empty.sh

LINK_LIB := $(AR)
LINK_LIB_FLAGS := rcs
LIBRARY_TO := 
POST_LINK_LIB := $(COMPILERPATH)/arm-none-eabi-gcc-ranlib

LINK_BIN := $(CXX)

LINK_BIN_FLAGS := -Os -Wl,--gc-sections,--defsym=__rtc_localtime=0 --specs=nano.specs -mcpu=$(CPUARCH) -mthumb -T$(MCU_LD) $(ARDUINO_OBJS)

FILE_SIZE := $(LS) $(LS_SIZE)
LOAD_SIZE := $(SIZE)

LINK_LIBS := -lm

OPT_SPEED := -Os
DEBUG := -g3

COVERAGE := -fprofile-arcs -ftest-coverage
DUMP = $(BUILD_ROOT)/mk/bin/empty.sh
MUNCH := $(BUILD_ROOT)/mk/bin/empty.sh

#Setup C/CXX C and CXX flags
CCXXFLAGS = -Wall -g -Os -mcpu=$(CPUARCH) -mthumb -MMD $(OPTIONS) $(ARDUINO_INCLUDES) $(COMMON_INCLUDES)
CFLAGS := $(CCXXFLAGS) $(DEBUG)
CXXFLAGS := $(CCXXFLAGS) -std=gnu++14 -felide-constructors -fno-exceptions -fno-rtti $(DEBUG)

#Set compiler name for output
COMPILER := gnu-debug-teensy-arduino-arm
