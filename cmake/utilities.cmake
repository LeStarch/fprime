####
# utilities.cmake:
#
# Utility and support functions for the fprime CMake build system.
####
include_guard()

####
# Function `plugin_name`:
#
# From a plugin include path retrieve the plugin name. This is the name without any .cmake extension.
#
# INCLUDE_PATH: path to plugin
# OUTPUT_VARIABLE: variable to set in caller's scope with result
####
function(plugin_name INCLUDE_PATH OUTPUT_VARIABLE)
    get_filename_component(TEMP_NAME "${INCLUDE_PATH}" NAME_WE)
    set("${OUTPUT_VARIABLE}" ${TEMP_NAME} PARENT_SCOPE)
endfunction(plugin_name)

####
# Function `generate_individual_function_call`:
#
# Generates a routing table entry for the faux cmake_language call for an individual function. This call consists of
# a single `elseif(name == function and ARGC == ARG_COUNT)` to support a call to the function with ARG_COUNT arguments.
# This is a helper function intended for use within `generate_faux_cmake_language`.
#
# OUTPUT_FILE: file to write these `elseif` blocks into
# FUNCTION: name of function to write out
# ARG_COUNT: number of args for this particular invocation of the call
####
function(generate_individual_function_call OUTPUT_FILE FUNCTION ARG_COUNT)
    # Build an invocation string of the form: ${FUNCTION}("${ARGV2}" "${ARGV3}" ..."${ARG_COUNT -1 + 2}")
    # Notice several properties:
    #     1. Calling function by a substituted name
    #     2. Arguments are specifically escaped. Thus **must** be done to ensure that empty, and list arguments are
    #        correctly handled. Otherwise they randomly expand or disappear
    #     3. Arg numbers start at 2. This accounts for ARGV0==CALL and ARGV1==Function name in the calling function
    set(ARG_STRING "")
    math(EXPR BOUND "${ARG_COUNT} - 1")
    foreach(ARG_IT RANGE "${BOUND}")
        math(EXPR ARG_NUM "${ARG_IT} + 2")
        set(ARG_STRING "${ARG_STRING} \"\${ARGV${ARG_NUM}}\"")
    endforeach()
    math(EXPR ARG_NUM "${ARG_COUNT} + 2")
    set(INVOCATION "${FUNCTION}(${ARG_STRING})")
    file(APPEND "${FAUX_FILE}" "    elseif (\"\${FUNCTION_NAME}\" STREQUAL ${FUNCTION} AND \${ARGC} EQUAL ${ARG_NUM})\n")
    file(APPEND "${FAUX_FILE}" "        ${INVOCATION}\n")
endfunction(generate_individual_function_call)

####
# Function `generate_faux_cmake_language`:
#
# This function is used to setup a fake implementation of `cmake_language` calls on implementations of CMake that
# predate its creation.  The facsimile is incomplete, but for the purposes of this build system, it will be sufficient
# meaning that it can route all the plugin functions correctly but specifically **not** arbitrary function calls.
#
# Functions supported by this call are expected in the GLOBAL property: CMAKE_LANGUAGE_ROUTE_LIST
#
# This is accomplished by writing out a CMake file that contains a macro that looks like the `cmake_language(CALL)`
# feature but is implemented by an `if (NAME == FUNCTION) FUNCTION() endif()` table. This file is built within and
# included when finished.
#
# In terms of performance:
#   - Native `cmake_language(CALL)` is incredibly fast
#   - This faux implementation is slow
#   - Repetitive including of .cmake files to "switch" implementations (as done in fprime v3.0.0) is **much** slower
####
function(generate_faux_cmake_language)
    set(FAUX_FILE "${CMAKE_BINARY_DIR}/cmake_language.cmake")
    set(ARG_MAX 5)
    file(WRITE  "${FAUX_FILE}" "#### AUTOGENERATED, DO NOT EDIT ####\n")
    file(APPEND "${FAUX_FILE}" "macro(cmake_language ACTION FUNCTION_NAME)\n")
    file(APPEND "${FAUX_FILE}" "    if (NOT \"\${ACTION}\" STREQUAL \"CALL\")\n")
    file(APPEND "${FAUX_FILE}" "        message(FATAL_ERROR \"Cannot use \${ACTION} with faux cmake_language\")\n")
    file(APPEND "${FAUX_FILE}" "    elseif (NOT COMMAND \"\${FUNCTION_NAME}\")\n")
    file(APPEND "${FAUX_FILE}" "        message(FATAL_ERROR \"Unknown function \${FUNCTION_NAME} supplied to faux cmake_language\")\n")
    # Generate one if block set for each function in the routing database
    get_property(FUNCTIONS GLOBAL PROPERTY CMAKE_LANGUAGE_ROUTE_LIST)
    foreach(FUNCTION IN LISTS FUNCTIONS)
        if (CMAKE_DEBUG_OUTPUT)
            math(EXPR ARG_TOP "${ARG_MAX} + 2")
            message(STATUS "Mimicking cmake_language(CALL ${FUNCTION} \"\${ARGV2}\" ... \"\${ARGV${ARG_TOP}}\"")
        endif()
        foreach(ARG_COUNT RANGE "${ARG_MAX}")
            generate_individual_function_call("${FAUX_FILE}" "${FUNCTION}" ${ARG_COUNT})
        endforeach()
        file(APPEND "${FAUX_FILE}" "    elseif (\"\${FUNCTION_NAME}\" STREQUAL ${FUNCTION})\n")
        file(APPEND "${FAUX_FILE}" "        message(FATAL_ERROR \"Faux cmake_language called with too-many arguments: \${ARGC}\")\n")
    endforeach()
    file(APPEND "${FAUX_FILE}" "    endif()\n")
    file(APPEND "${FAUX_FILE}" "endmacro(cmake_language)\n")
    include("${FAUX_FILE}")
endfunction()

####
# Function `plugin_include_helper`:
#
# Designed to help include API files (targets, autocoders) in an efficient way within CMake. This function imports a
# CMake file and defines a `dispatch_<function>(PLUGIN_NAME ...)` function for each function name in ARGN. Thus users
# of the imported plugin can call `dispatch_<function>(PLUGIN_NAME ...)` to dispatch a function as implemented in a
# plugin.
#
# OUTPUT_VARIABLE: set with the plugin name that has last been included
# INCLUDE_PATH: path to file to include
####
function(plugin_include_helper OUTPUT_VARIABLE INCLUDE_PATH)
    plugin_name("${INCLUDE_PATH}" PLUGIN_NAME)
    # Get the global property of all function items
    get_property(TEMP_LIST GLOBAL PROPERTY CMAKE_LANGUAGE_ROUTE_LIST)
    set(CHANGED FALSE)
    foreach(PLUGIN_FUNCTION IN LISTS ARGN)
        # Include the file if we have not found the prefixed function name yet
        if (NOT COMMAND "${PLUGIN_NAME}_${PLUGIN_FUNCTION}")
            include("${INCLUDE_PATH}")
        endif()
        # Add the function if any of the set were determined missing
        if (NOT "${PLUGIN_NAME}_${PLUGIN_FUNCTION}" IN_LIST TEMP_LIST)
            set_property(GLOBAL APPEND PROPERTY CMAKE_LANGUAGE_ROUTE_LIST "${PLUGIN_NAME}_${PLUGIN_FUNCTION}")
            set(CHANGED TRUE)
        endif()
    endforeach()

    # If cmake_language is not available, we have to implement it
    if(CHANGED AND ${CMAKE_VERSION} VERSION_LESS "3.18.0")
        generate_faux_cmake_language()
    endif()
    set("${OUTPUT_VARIABLE}" "${PLUGIN_NAME}" PARENT_SCOPE)
endfunction(plugin_include_helper)

####
# starts_with:
#
# Check if the string input starts with the given prefix. Sets OUTPUT_VAR to TRUE when it does and sets OUTPUT_VAR to
# FALSE when it does not. OUTPUT_VAR is the name of the variable in PARENT_SCOPE that will be set.
#
# Note: regexs in CMake are known to be inefficient. Thus `starts_with` and `ends_with` are implemented without them
# in order to ensure speed.
#
# OUTPUT_VAR: variable to set
# STRING: string to check
# PREFIX: expected ending
####
function(starts_with OUTPUT_VAR STRING PREFIX)
    set("${OUTPUT_VAR}" FALSE PARENT_SCOPE)
    string(LENGTH "${PREFIX}" PREFIX_LENGTH)
    string(SUBSTRING "${STRING}" "0" "${PREFIX_LENGTH}" FOUND_PREFIX)
    # Check the substring
    if (FOUND_PREFIX STREQUAL "${PREFIX}")
        set("${OUTPUT_VAR}" TRUE PARENT_SCOPE)
    endif()
endfunction(starts_with)

####
# ends_with:
#
# Check if the string input ends with the given suffix. Sets OUTPUT_VAR to TRUE when it does and  sets OUTPUT_VAR to
# FALSE when it does not. OUTPUT_VAR is the name of the variable in PARENT_SCOPE that will be set.
#
# Note: regexs in CMake are known to be inefficient. Thus `starts_with` and `ends_with` are implemented without them
# in order to ensure speed.
#
# OUTPUT_VAR: variable to set
# STRING: string to check
# SUFFIX: expected ending
####
function(ends_with OUTPUT_VAR STRING SUFFIX)
    set("${OUTPUT_VAR}" FALSE PARENT_SCOPE)
    string(LENGTH "${STRING}" INPUT_LENGTH)
    string(LENGTH "${SUFFIX}" SUFFIX_LENGTH)
    if (INPUT_LENGTH GREATER_EQUAL SUFFIX_LENGTH)
        # Calculate the substring of suffix length at end of string
        math(EXPR START "${INPUT_LENGTH} - ${SUFFIX_LENGTH}")
        string(SUBSTRING "${STRING}" "${START}" "${SUFFIX_LENGTH}" FOUND_SUFFIX)
        # Check the substring
        if (FOUND_SUFFIX STREQUAL "${SUFFIX}")
            set("${OUTPUT_VAR}" TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction(ends_with)

####
# init_variables:
#
# Initialize all variables passed in to empty variables in the calling scope.
####
function(init_variables)
    foreach (VARIABLE IN LISTS ARGN)
        set(${VARIABLE} "" PARENT_SCOPE)
    endforeach()
endfunction(init_variables)

####
# normalize_paths:
#
# Take in any number of lists of paths and normalize the paths returning a single list.
# OUTPUT_NAME: name of variable to set in parent scope
####
function(normalize_paths OUTPUT_NAME)
    set(OUTPUT_LIST)
    # Loop over the list and check
    foreach (PATH_LIST IN LISTS ARGN)
        foreach(PATH IN LISTS PATH_LIST)
            get_filename_component(PATH "${PATH}" REALPATH)
            list(APPEND OUTPUT_LIST "${PATH}")
        endforeach()
    endforeach()
    set(${OUTPUT_NAME} "${OUTPUT_LIST}" PARENT_SCOPE)
endfunction(normalize_paths)

####
# resolve_dependencies:
#
# Sets OUTPUT_VAR in parent scope to be the set of dependencies in canonical form: relative path from root replacing
# directory separators with "_".  E.g. fprime/Fw/Time becomes Fw_Time.
#
# OUTPUT_VAR: variable to fill in parent scope
# ARGN: list of dependencies to resolve
####
function(resolve_dependencies OUTPUT_VAR)
    # Resolve all dependencies
    set(RESOLVED)
    foreach(DEPENDENCY IN LISTS ARGN)
        # No resolution is done on linker-only dependencies
        linker_only(LINKER_ONLY "${DEPENDENCY}")
        if (LINKER_ONLY)
            list(APPEND RESOLVED "${DEPENDENCY}")
            continue()
        endif()
        get_module_name(${DEPENDENCY})
        if (NOT MODULE_NAME IN_LIST RESOLVED)
            list(APPEND RESOLVED "${MODULE_NAME}")
        endif()
    endforeach()
    set(${OUTPUT_VAR} "${RESOLVED}" PARENT_SCOPE)
endfunction(resolve_dependencies)

####
# Function `is_target_real`:
#
# Does this target represent a real item (executable, library)? OUTPUT is set to TRUE when real, and FALSE otherwise.
#
# OUTPUT: variable to set
# TEST_TARGET: target to set
####
function(is_target_real OUTPUT TEST_TARGET)
    if (TARGET "${DEPENDENCY}")
        get_target_property(TARGET_TYPE "${DEPENDENCY}" TYPE)
        # Make sure this is not a utility target
        if (NOT TARGET_TYPE STREQUAL "UTILITY")
            set("${OUTPUT}" TRUE PARENT_SCOPE)
            return()
        endif()
    endif()
    set("${OUTPUT}" FALSE PARENT_SCOPE)
endfunction()

####
# Function `is_target_library`:
#
# Does this target represent a real library? OUTPUT is set to TRUE when real, and FALSE otherwise.
#
# OUTPUT: variable to set
# TEST_TARGET: target to set
####
function(is_target_library OUTPUT TEST_TARGET)
    set("${OUTPUT}" FALSE PARENT_SCOPE)
    if (TARGET "${TEST_TARGET}")
        get_target_property(TARGET_TYPE "${DEPENDENCY}" TYPE)
        ends_with(IS_LIBRARY "${TARGET_TYPE}" "_LIBRARY")
        set("${OUTPUT}" "${IS_LIBRARY}" PARENT_SCOPE)
    endif()
endfunction()

####
# linker_only:
#
# Checks if a given dependency should be supplied to the linker only. These will not be supplied as CMake dependencies
# but will be supplied as link libraries. These tokens are of several types:
#
# 1. Linker flags: starts with -l
# 2. Existing Files: accounts for preexisting libraries shared and otherwise
#
# OUTPUT_VAR: variable to set in PARENT_SCOPE to TRUE/FALSE
# TOKEN: token to check if "linker only"
####
function(linker_only OUTPUT_VAR TOKEN)
    set("${OUTPUT_VAR}" FALSE PARENT_SCOPE)
    starts_with(IS_LINKER_FLAG "${TOKEN}" "-l")
    if (IS_LINKER_FLAG OR (EXISTS "${TOKEN}" AND NOT IS_DIRECTORY "${TOKEN}"))
        set("${OUTPUT_VAR}" TRUE PARENT_SCOPE)
    endif()
endfunction()

####
# build_relative_path:
#
# Calculate the path to an item relative to known build paths.  Search is performed in the following order erring if the
# item is found in multiple paths.
#
# INPUT_PATH: input path to search
# OUTPUT_VAR: output variable to fill
####
function(build_relative_path INPUT_PATH OUTPUT_VAR)
    # Implementation assertion
    if (NOT DEFINED FPRIME_BUILD_LOCATIONS)
        message(FATAL_ERROR "FPRIME_BUILD_LOCATIONS not set before build_relative_path was called")
    endif()
    normalize_paths(FPRIME_LOCS_NORM ${FPRIME_BUILD_LOCATIONS})
    normalize_paths(INPUT_PATH ${INPUT_PATH})
    foreach(PARENT IN LISTS FPRIME_LOCS_NORM)
        string(REGEX REPLACE "${PARENT}/(.*)$" "\\1" LOC_TEMP "${INPUT_PATH}")
        if (NOT LOC_TEMP STREQUAL INPUT_PATH AND NOT LOC_TEMP MATCHES "${LOC}$")
            message(FATAL_ERROR "Found ${INPUT_PATH} at multiple locations: ${LOC} and ${LOC_TEMP}")
        elseif(NOT LOC_TEMP STREQUAL INPUT_PATH AND NOT DEFINED LOC)
            set(LOC "${LOC_TEMP}")
        endif()
    endforeach()
    if (LOC STREQUAL "")
        message(FATAL_ERROR "Failed to find location for: ${INPUT_PATH}")
    endif()
    set(${OUTPUT_VAR} ${LOC} PARENT_SCOPE)
endfunction(build_relative_path)

####
# on_any_changed:
#
# Sets VARIABLE to true if any file has been noted as changed from the "on_changed" function.  Will create cache files
# in the binary directory.  Please see: on_changed
#
# INPUT_FILES: files to check for changes
# ARGN: passed into execute_process via on_changed call
####
function (on_any_changed INPUT_FILES VARIABLE)
    foreach(INPUT_FILE IN LISTS INPUT_FILES)
        on_changed("${INPUT_FILE}" TEMP_ON_CHANGED ${ARGN})
        if (TEMP_ON_CHANGED)
            set(${VARIABLE} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${VARIABLE} FALSE PARENT_SCOPE)
endfunction()

####
# on_changed:
#
# Sets VARIABLE to true if and only if the given file has changed since the last time this function was invoked. It will
# create "${INPUT_FILE}.prev" in the binary directory as a cache from the previous invocation. The result is always TRUE
# unless a successful no-difference is calculated.
#
# INPUT_FILE: file to check if it has changed
# ARGN: passed into execute_process
####
function (on_changed INPUT_FILE VARIABLE)
    get_filename_component(INPUT_BASENAME "${INPUT_FILE}" NAME)
    set(PREVIOUS_FILE "${CMAKE_CURRENT_BINARY_DIR}/${INPUT_BASENAME}.prev")

    execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${INPUT_FILE}" "${PREVIOUS_FILE}"
                    RESULT_VARIABLE difference OUTPUT_QUIET ERROR_QUIET)
    # Files are the same, leave this function
    if (difference EQUAL 0)
        set(${VARIABLE} FALSE PARENT_SCOPE)
        return()
    endif()
    set(${VARIABLE} TRUE PARENT_SCOPE)
    # Update the file with the latest
    if (EXISTS "${INPUT_FILE}")
        execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${INPUT_FILE}" "${PREVIOUS_FILE}" OUTPUT_QUIET)
    endif()
endfunction()

####
# read_from_lines:
#
# Reads a set of variables from a newline delimited test base. This will read each variable as a separate line. It is
# based on the number of arguments passed in.
####
function (read_from_lines CONTENT)
    # Loop through each arg
    foreach(NAME IN LISTS ARGN)
        string(REGEX MATCH   "^([^\r\n]+)" VALUE "${CONTENT}")
        string(REGEX REPLACE "^([^\r\n]*)\r?\n(.*)" "\\2" CONTENT "${CONTENT}")
        set(${NAME} "${VALUE}" PARENT_SCOPE)
    endforeach()
endfunction()

####
# Function `full_path_from_build_relative_path`:
#
# Creates a full path from the shortened build-relative path.
# -**SHORT_PATH:** build relative path
# Return: full path from relative path
####
function(full_path_from_build_relative_path SHORT_PATH OUTPUT_VARIABLE)
    foreach(FPRIME_LOCATION IN LISTS FPRIME_BUILD_LOCATIONS)
        if (EXISTS "${FPRIME_LOCATION}/${SHORT_PATH}")
            set("${OUTPUT_VARIABLE}" "${FPRIME_LOCATION}/${SHORT_PATH}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set("${OUTPUT_VARIABLE}" "" PARENT_SCOPE)
endfunction(full_path_from_build_relative_path)

####
# Function `get_nearest_build_root`:
#
# Finds the nearest build root from ${FPRIME_BUILD_LOCATIONS} that is a parent of DIRECTORY_PATH.
#
# - **DIRECTORY_PATH:** path to detect nearest build root
# Return: nearest parent from ${FPRIME_BUILD_LOCATIONS}
####
function(get_nearest_build_root DIRECTORY_PATH)
    set(FOUND_BUILD_ROOT "${DIRECTORY_PATH}")
    set(LAST_REL "${DIRECTORY_PATH}")
    foreach(FPRIME_BUILD_LOC ${FPRIME_BUILD_LOCATIONS})
        file(RELATIVE_PATH TEMP_MODULE ${FPRIME_BUILD_LOC} ${DIRECTORY_PATH})
        string(LENGTH "${LAST_REL}" LEN1)
        string(LENGTH "${TEMP_MODULE}" LEN2)
        if (LEN2 LESS LEN1 AND TEMP_MODULE MATCHES "^[^./].*")
            set(FOUND_BUILD_ROOT "${FPRIME_BUILD_LOC}")
            set(LAST_REL "${TEMP_MODULE}")
        endif()
    endforeach()
    if ("${FOUND_BUILD_ROOT}" STREQUAL "${DIRECTORY_PATH}")
        message(FATAL_ERROR "No build root found for: ${DIRECTORY_PATH}")
    endif()
    set(FPRIME_CLOSEST_BUILD_ROOT "${FOUND_BUILD_ROOT}" PARENT_SCOPE)
endfunction()
####
# Function `get_module_name`:
#
# Takes a path, or something path-like and returns the module's name. This breaks down as the
# following:
#
#  1. If passed a path, the module name is the '_'ed variant of the relative path from BUILD_ROOT
#  2. If passes something which does not exist on the file system, it is just '_'ed
#
# i.e. ${BUILD_ROOT}/Svc/ActiveLogger becomes Svc_ActiveLogger
#      Svc/ActiveLogger also becomes Svc_ActiveLogger
#
# - **DIRECTORY_PATH:** (optional) path to infer MODULE_NAME from. Default: CMAKE_CURRENT_LIST_DIR
# - **Return: MODULE_NAME** (set in parent scope)
####
function(get_module_name)
    # Set optional arguments
    if (ARGN)
        set(DIRECTORY_PATH "${ARGN}")
    else()
        set(DIRECTORY_PATH "${CMAKE_CURRENT_LIST_DIR}")
    endif()
    # If DIRECTORY_PATH exists, then find its offset from BUILD_ROOT to calculate the module
    # name. If it does not exist, then it is assumed to be an offset already and is carried
    # forward in the calculation.
    if (EXISTS ${DIRECTORY_PATH} AND IS_ABSOLUTE ${DIRECTORY_PATH})
        # Module names a based on the current directory, not a file
        if (NOT IS_DIRECTORY ${DIRECTORY_PATH})
            get_filename_component(DIRECTORY_PATH "${DIRECTORY_PATH}" DIRECTORY)
        endif()
        # Get path name relative to the root directory
        get_nearest_build_root(${DIRECTORY_PATH})
        File(RELATIVE_PATH TEMP_MODULE_NAME ${FPRIME_CLOSEST_BUILD_ROOT} ${DIRECTORY_PATH})
    else()
        set(TEMP_MODULE_NAME ${DIRECTORY_PATH})
    endif()
    # Replace slash with underscore to have valid name
    string(REPLACE "/" "_" TEMP_MODULE_NAME ${TEMP_MODULE_NAME})
    set(MODULE_NAME ${TEMP_MODULE_NAME} PARENT_SCOPE)
endfunction(get_module_name)

####
# Function `get_expected_tool_version`:
#
# Gets the expected tool version named using version identifier VID to name the tools package
# file. This will be returned via the variable supplied in FILL_VARIABLE setting it in PARENT_SCOPE.
####
function(get_expected_tool_version VID FILL_VARIABLE)
    find_program(TOOLS_CHECK NAMES fprime-version-check REQUIRED)
    
    # Try project root as a source
    set(REQUIREMENT_FILE "${FPRIME_PROJECT_ROOT}/requirements.txt")
    if (EXISTS "${REQUIREMENT_FILE}")
        execute_process(COMMAND "${TOOLS_CHECK}" "${VID}" "${REQUIREMENT_FILE}" OUTPUT_VARIABLE VERSION_TEXT ERROR_VARIABLE ERRORS RESULT_VARIABLE RESULT_OUT OUTPUT_STRIP_TRAILING_WHITESPACE)
        if (CMAKE_DEBUG_OUTPUT)
            message(STATUS "[VERSION] Could not detect version from: ${REQUIREMENT_FILE}. ${ERRORS}")
        endif()
        if (RESULT_OUT EQUAL 0)
            set("${FILL_VARIABLE}" "${VERSION_TEXT}" PARENT_SCOPE)
            return()
        endif()
    endif()
    # Fallback to requirements.txt in fprime
    set(REQUIREMENT_FILE "${FPRIME_FRAMEWORK_PATH}/requirements.txt")
    execute_process(COMMAND "${TOOLS_CHECK}" "${VID}" "${REQUIREMENT_FILE}" OUTPUT_VARIABLE VERSION_TEXT ERROR_VARIABLE ERRORS RESULT_VARIABLE RESULT_OUT OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (RESULT_OUT EQUAL 0)
        set("${FILL_VARIABLE}" "${VERSION_TEXT}" PARENT_SCOPE)
        return()
    endif()
    message(WARNING "[VERSION] Could not detect version from: ${REQUIREMENT_FILE}. ${ERRORS}. Skipping check.")
    set("${FILL_VARIABLE}" "" PARENT_SCOPE)
endfunction(get_expected_tool_version)

####
# Function `set_assert_flags`:
#
# Adds a -DASSERT_FILE_ID=(First 8 digits of MD5) to each source file, and records the output in
# hashes.txt. This allows for asserts on file ID not string. Also adds the -DASSERT_RELATIVE_PATH
# flag for handling relative path asserts.
####
function(set_assert_flags SRC)
    get_filename_component(FPRIME_CLOSEST_BUILD_ROOT_ABS "${FPRIME_CLOSEST_BUILD_ROOT}" ABSOLUTE)
    get_filename_component(FPRIME_PROJECT_ROOT_ABS "${FPRIME_PROJECT_ROOT}" ABSOLUTE)
    string(REPLACE "${FPRIME_CLOSEST_BUILD_ROOT_ABS}/" "" SHORT_SRC "${SRC}")
    string(REPLACE "${FPRIME_PROJECT_ROOT_ABS}/" "" SHORT_SRC "${SHORT_SRC}")

    string(MD5 HASH_VAL "${SHORT_SRC}")
    string(SUBSTRING "${HASH_VAL}" 0 8 HASH_32)
    file(APPEND "${CMAKE_BINARY_DIR}/hashes.txt" "${SHORT_SRC}: 0x${HASH_32}\n")
    SET_SOURCE_FILES_PROPERTIES(${SRC} PROPERTIES COMPILE_FLAGS "-DASSERT_FILE_ID=0x${HASH_32} -DASSERT_RELATIVE_PATH='\"${SHORT_SRC}\"'")
endfunction(set_assert_flags)


####
# Function `print_property`:
#
# Prints a given property for the module.
# - **TARGET**: target to print properties
# - **PROPERTY**: name of property to print
####
function (print_property TARGET PROPERTY)
    get_target_property(OUT "${TARGET}" "${PROPERTY}")
    if (NOT OUT MATCHES ".*-NOTFOUND")
        message(STATUS "[F´ Module] ${TARGET} ${PROPERTY}:")
        foreach (PROPERTY IN LISTS OUT)
            message(STATUS "[F´ Module]    ${PROPERTY}")
        endforeach()
    endif()
endfunction(print_property)

####
# Function `introspect`:
#
# Prints the dependency list of the module supplied as well as the include directories.
#
# - **MODULE_NAME**: module name to print dependencies
####
function(introspect MODULE_NAME)
    print_property("${MODULE_NAME}" SOURCES)
    print_property("${MODULE_NAME}" INCLUDE_DIRECTORIES)
    print_property("${MODULE_NAME}" LINK_LIBRARIES)
endfunction(introspect)

####
# Function `execute_process_or_fail`:
#
# Calls CMake's `execute_process` with the arguments passed in via ARGN. This call is wrapped to print out the command
# line invocation when CMAKE_DEBUG_OUTPUT is set ON, and will check that the command processes correctly.  Any error
# message is output should the command fail. No handling is done of standard error.
#
# Errors are determined by checking the process's return code where a FATAL_ERROR is produced on non-zero.
#
# - **ERROR_MESSAGE**: message to output should an error occurs
####
function(execute_process_or_fail ERROR_MESSAGE)
    # Quiet standard output unless we are doing verbose output generate
    set(OUTPUT_ARGS OUTPUT_QUIET)
    # Print the invocation if debug output is set
    if (CMAKE_DEBUG_OUTPUT)
        set(OUTPUT_ARGS)
        set(COMMAND_AS_STRING "")
        foreach(ARG IN LISTS ARGN)
            set(COMMAND_AS_STRING "${COMMAND_AS_STRING}\"${ARG}\" ")
        endforeach()
        
        #string(REPLACE ";" "\" \"" COMMAND_AS_STRING "${ARGN}")
        message(STATUS "[cli] ${COMMAND_AS_STRING}")
    endif()
    execute_process(
        COMMAND ${ARGN}
        RESULT_VARIABLE RETURN_CODE
        ERROR_VARIABLE STANDARD_ERROR
        ERROR_STRIP_TRAILING_WHITESPACE
        ${OUTPUT_ARGS}
    )
    if (NOT RETURN_CODE EQUAL 0)
        message(FATAL_ERROR "${ERROR_MESSAGE}:\n${STANDARD_ERROR}")
    endif()
endfunction()

####
# Function `append_list_property`:
#
# Appends the NEW_ITEM to a property. ARGN is a set of arguments that are passed into the get and set property calls.
# This function calls get_property with ARGN appends NEW_ITEM to the result and then turns around and calls set_property
# with the new list. Callers **should not** supply the variable name argument to get_property.
#
# Duplicate entries are removed.
#
# Args:
# - `NEW_ITEM`: item to append to the property
# - `ARGN`: list of arguments forwarded to get and set property calls.
####
function(append_list_property NEW_ITEM)
    get_property(LOCAL_COPY ${ARGN})
    list(APPEND LOCAL_COPY "${NEW_ITEM}")
    list(REMOVE_DUPLICATES LOCAL_COPY)
    set_property(${ARGN} "${LOCAL_COPY}")
endfunction()
