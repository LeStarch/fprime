####
# default/config/CMakeLists.txt:
#
# Sets a list of source files for cmake to process as part of autocoding.
####

####
# WARNING: this configuration module is intended to be used only for configuration that is independent of the Fw_Types
# module. That means in cannot include any files that reference types in Fw_Types. It also cannot include the
# configuration for any components nor ports as they build on Fw_Comp / Fw_Port packages, which use Fw_Types.
####
register_fprime_config(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/AcConstants.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/FpConfig.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/FpConstants.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/OsCfg.fpp"
        "${CMAKE_CURRENT_LIST_DIR}/PlatformCfg.fpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/FpConfig.h"
        "${CMAKE_CURRENT_LIST_DIR}/FpConfig.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/FPrimeNumericalConfig.h"
    BASE_CONFIG
)
