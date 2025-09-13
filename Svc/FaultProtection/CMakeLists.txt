register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/ClearFaultEnum.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/FaultProtection.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/FaultProtection.cpp"
    DEPENDS
        Os
)

register_fprime_ut(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/FaultProtection.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/TestMain.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test/ut/FaultProtectionTester.cpp"
)
