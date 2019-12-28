##
# Teensyduino.cmake:
#
# A platform file supporting the Teensy chip. This is to parallel the toolchain file
# of the same name.
##

# RPI is Linux based, so include the common Linux items
include_directories("${FPRIME_CORE_DIR}/Fw/Types/Arduino")
