####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding diles
# MOD_DEPS: (optional) module dependencies
#
####
(NOT DEFINED FPRIME_USE_STUBBED_DRIVERS OR FPRIME_USE_STUBBED_DRIVERS)
    (SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/LinuxGpioDriverComponentAi.xml"
        "${CMAKE_CURRENT_LIST_DIR}/LinuxGpioDriverComponentImplCommon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/LinuxGpioDriverComponentImplStub.cpp"
    )
(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    (SOURCE_FILES
        "${CMAKE_CURRENT_LIST_DIR}/LinuxGpioDriverComponentAi.xml"
        "${CMAKE_CURRENT_LIST_DIR}/LinuxGpioDriverComponentImplCommon.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/LinuxGpioDriverComponentImpl.cpp"
    )
()
    message(STATUS "Cannot use ${CMAKE_CURRENT_LIST_DIR} with platform ${CMAKE_SYSTEM_NAME}. Skipping.")
()

register_fprime_module()
