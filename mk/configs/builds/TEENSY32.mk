COMP ?= DEFAULT

TEENSY = teensy32

ifeq ($(COMP),DEFAULT)
 include $(BUILD_ROOT)/mk/configs/builds/teensy-arduino-gcc-debug.mk
endif

ifeq ($(COMP),gcc-debug)
 include $(BUILD_ROOT)/mk/configs/builds/teensy-arduino-gcc-debug.mk
endif

# Implementation deferred
#ifeq ($(COMP),comp-ut)
# include $(BUILD_ROOT)/mk/configs/builds/
#endif
#
#ifeq ($(COMP),comp-ut-nocov)
# include $(BUILD_ROOT)/mk/configs/builds/
#endif
