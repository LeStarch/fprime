####
# 'ArduinoBlink' Deployment:
#
# This sets up the  ArduinoBLink example using CMake for use with F ptime. It should serve as an
# example for those who wish to use Arduino support.
#
# This file has several sections.
#
# 1. Header Section: define basic properties of the build
# 2. F´ core: includes all F´ core components, and build-system properties
# 3. Local subdirectories: contains all deployment specific directory additions
####

##
# Section 1: Basic Project Setup
#
# This contains the basic project information. Specifically, a cmake version and
# project definition.
##
project(ArduinoBlink C CXX ASM)
cmake_minimum_required(VERSION 3.5)

set(BAREMETAL_SCHEDULER CACHE BOOL ON "Uses the baremtal scheduler")
##
# Section 2: F´ Core
#
# This includes all of the F´ core components, and imports the make-system. F´ core
# components will be placed in the F-Prime binary subdirectory to keep them from
# colliding with deployment specific items.
##
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/FPrime.cmake")
# NOTE: register custom targets between these two lines
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/FPrime-Code.cmake")
# Note: when building a deployment outside of the F´ core directories, then the
# build root must be re-mapped for use with the standard build system components.
#
# In this way, the module names can be predicted as an offset from the (new) build
# root, without breaking the standard locations of F´.
#
# Uncomment the following lines, and set them to the BUILD_ROOT of your deployment,
# which is typically one directory up from the CMakeLists.txt in the deployment dir.
#set(FPRIME_CURRENT_BUILD_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
#message(STATUS "F´ BUILD_ROOT currently set to: ${FPRIME_CURRENT_BUILD_ROOT}")

##
# Section 3: Components and Topology
#
# This section includes deployment specific directories. This allows use of non-
# core components in the topology, which is also added here.
##
# Add component subdirectories
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../ArduinoGpsTracker/SerialDriver/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../ArduinoGpsTracker/HardwareRateDriver/")
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../ArduinoGpsTracker/LedBlinker/")

# Add Topology subdirectory
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top/")

