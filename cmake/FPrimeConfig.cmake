# Helper not available as fprime cmake code not loaded yet
if (NOT FPRIME_CMAKE_QUIET)
    message(STATUS "[F Prime] F Prime CMake package found at: ${CMAKE_CURRENT_LIST_FILE}")
endif()
include("${CMAKE_CURRENT_LIST_DIR}/FPrime.cmake")

# Set the FPRIME_FRAMEWORK_PATH to the parent directory of this file as an absolute path
if (NOT DEFINED FPRIME_FRAMEWORK_PATH)
    set(FPRIME_FRAMEWORK_PATH "${CMAKE_CURRENT_LIST_DIR}/.." )
    get_filename_component(FPRIME_FRAMEWORK_PATH "${FPRIME_FRAMEWORK_PATH}" ABSOLUTE)
endif()

# Set the FPRIME_PROJECT_ROOT to the parent directory of this file as an absolute path
if (NOT DEFINED FPRIME_PROJECT_ROOT)
    get_filename_component(FPRIME_PROJECT_ROOT "${PROJECT_SOURCE_DIR}" ABSOLUTE)
endif()

# By default the F Prime package will load the codebase. This can be set OFF by setting the variable
# FPRIME_INCLUDE_FRAMEWORK_CODE to OFF. When set OFF, the user must call fprime_setup_included_code()
# to be able to use F Prime code. 
if ((NOT DEFINED FPRIME_INCLUDE_FRAMEWORK_CODE) OR (FPRIME_INCLUDE_FRAMEWORK_CODE))
    fprime_setup_included_code()    
endif()
