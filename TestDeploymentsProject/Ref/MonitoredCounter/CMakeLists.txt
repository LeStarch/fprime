####
# F Prime CMakeLists.txt:
#
# SOURCES: list of source files (to be compiled)
# AUTOCODER_INPUTS: list of files to be passed to the autocoders
# DEPENDS: list of libraries that this module depends on
#
# More information in the F´ CMake API documentation:
# https://fprime.jpl.nasa.gov/latest/docs/reference/api/cmake/API/
#
####
register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/MonitoredCounter.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/MonitoredCounter.cpp"
    DEPENDS
        Svc_FaultProtection_Interfaces
        Svc_FaultProtection_FaultConfig
)

### Unit Tests ###
register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/MonitoredCounter.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/MonitoredCounterTestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/MonitoredCounterTester.cpp"
    UT_AUTO_HELPERS
)
