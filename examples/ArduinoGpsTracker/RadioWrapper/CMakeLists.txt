####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding diles
# MOD_DEPS: (optional) module dependencies
#
####
set(SOURCE_FILES
    "${CMAKE_CURRENT_LIST_DIR}/RadioWrapperComponentAi.xml"
    "${CMAKE_CURRENT_LIST_DIR}/RadioWrapper.cpp"
)
# Check the Linux build
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Teensyduino")
    list(APPEND SOURCE_FILES "${CMAKE_CURRENT_LIST_DIR}/RadioWrapperLinux.cpp")
# Teensyduino additions
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Teensyduino")
    list(APPEND SOURCE_FILES 
        "${CMAKE_CURRENT_LIST_DIR}/RadioWrapperArduino.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/../RFM69/RFM69.cpp")
endif()
register_fprime_module()
target_include_directories("examples_ArduinoGpsTracker_RadioWrapper" PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../RFM69")
# Add Teensyduino dependency
if(${CMAKE_SYSTEM_NAME} STREQUAL "Teensyduino")
    get_module_name(${CMAKE_CURRENT_LIST_DIR})
    add_arduino_dependency("${MODULE_NAME}")
endif()
