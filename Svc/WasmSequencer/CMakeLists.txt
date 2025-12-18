####
# F prime CMakeLists.txt:
#
# SOURCE_FILES: combined list of source and autocoding files
# MOD_DEPS: (optional) module dependencies
# UT_SOURCE_FILES: list of source files for unit tests
#
####
#set(ENGINE WASR)
set(ENGINE WASM3)

if ("${ENGINE}" STREQUAL WASR)

    set (WAMR_BUILD_PLATFORM "darwin")
    set (WAMR_BUILD_TARGET "AARCH64")
    set (WAMR_BUILD_INTERP 1)
    set (WAMR_BUILD_JIT 0)
    set (WAMR_BUILD_FAST_JIT 0)
    set (WAMR_BUILD_FAST_INTERP 0)
    set (WAMR_BUILD_AOT 0)
    set (WAMR_BUILD_LIBC_BUILTIN 1)
    set (WAMR_BUILD_LIBC_WASI 0)
    set (WAMR_BUILD_SIMD 0)
    set (WAMR_BUILD_MULTI_MODULE 0)
    set (WAMR_BUILD_MINI_LOADER 0)
    set (WAMR_BUILD_SHARED_MEMORY 0)
    set (WAMR_BUILD_THREAD_MGR 0)
    set (WAMR_BUILD_LIB_PTHREAD 0)
    set (WAMR_BUILD_LIB_PTHREAD_SEMAPHORE 0)
    set (WAMR_BUILD_LIB_WASI_THREADS 0)
    set (WAMR_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/wasm-micro-runtime")

    include (${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
    add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})
    target_compile_options(vmlib PRIVATE -Wno-extra-semi -Wno-strict-prototypes -Wno-newline-eof -Wno-c11-extensions -Wno-sign-conversion)
    
    # include bh_read_file.h
    include (${SHARED_DIR}/utils/uncommon/shared_uncommon.cmake)

    add_executable("${FPRIME_CURRENT_MODULE}" "${CMAKE_CURRENT_LIST_DIR}/Main.cpp" ${UNCOMMON_SHARED_SOURCE})
    target_link_libraries("${FPRIME_CURRENT_MODULE}" vmlib -lm)
elseif("${ENGINE}" STREQUAL WASM3)
    #set(CMAKE_CXX_STANDARD 17)
    # Add the WASM3 subdirectories for WASM3 and WASM3_CPP
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/wasm3/source ${CMAKE_BINARY_DIR}/wasm3-m3)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/wasm3/platforms/cpp/wasm3_cpp)
    
    add_executable("${FPRIME_CURRENT_MODULE}_sample" "${CMAKE_CURRENT_LIST_DIR}/Main-Wasm3.cpp" ${UNCOMMON_SHARED_SOURCE})
    target_link_libraries("${FPRIME_CURRENT_MODULE}_sample" PUBLIC m3 wasm3_cpp)
    target_compile_options(m3 PUBLIC -Wno-old-style-cast -Wno-error)
    target_compile_options("${FPRIME_CURRENT_MODULE}_sample" PRIVATE -Wno-old-style-cast)
    # Line is needed for the test program header
    target_include_directories("${FPRIME_CURRENT_MODULE}_sample" PRIVATE "${CMAKE_CURRENT_LIST_DIR}/wasm3/platforms/cpp")
endif()

register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/WasmSequencer.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/WasmFunctions.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/WasmSequencer.cpp"
    DEPENDS
        m3
)

#register_fprime_ut()
