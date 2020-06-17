include(CheckCXXSourceCompiles)

function(qt_run_config_test_architecture)
    set(QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT
        "" CACHE INTERNAL "Test variables that should be exported" FORCE)

    # Compile test to find the target architecture and sub-architectures.
    set(flags "")
    qt_get_platform_try_compile_vars(platform_try_compile_vars)
    list(APPEND flags ${platform_try_compile_vars})

    try_compile(
        _arch_result
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/arch"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/arch"
        arch
        CMAKE_FLAGS ${flags}
        )

    if (NOT _arch_result)
        message(FATAL_ERROR "Failed to compile architecture detection file.")
    endif()

    set(_arch_file_suffix "${CMAKE_EXECUTABLE_SUFFIX}")
    # With emscripten the application entry point is a .js file (to be run with node for example),
    # but the real "data" is in the .wasm file, so that's where we need to look for the ABI, etc.
    # information.
    if (EMSCRIPTEN)
        set(_arch_file_suffix ".wasm")
    endif()

    set(arch_test_location "config.tests/arch")
    if(QT_MULTI_CONFIG_FIRST_CONFIG)
        string(APPEND arch_test_location "/${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()

    set(_arch_file
        "${CMAKE_CURRENT_BINARY_DIR}/${arch_test_location}/architecture_test${_arch_file_suffix}")
    if (NOT EXISTS "${_arch_file}")
        message(FATAL_ERROR
                "Failed to find compiled architecture detection executable at ${_arch_file}.")
    endif()
    message(STATUS "Extracting architecture info from ${_arch_file}.")

    file(STRINGS "${_arch_file}" _arch_lines LENGTH_MINIMUM 16 LENGTH_MAXIMUM 1024 ENCODING UTF-8)

    foreach (_line ${_arch_lines})
        string(LENGTH "${_line}" lineLength)
        string(FIND "${_line}" "==Qt=magic=Qt== Architecture:" _pos)
        if (_pos GREATER -1)
            math(EXPR _pos "${_pos}+29")
            string(SUBSTRING "${_line}" ${_pos} -1 _architecture)
        endif()
        string(FIND "${_line}" "==Qt=magic=Qt== Sub-architecture:" _pos)
        if (_pos GREATER -1 AND ${lineLength} GREATER 33)
            math(EXPR _pos "${_pos}+34")
            string(SUBSTRING "${_line}" ${_pos} -1 _sub_architecture)
            string(REPLACE " " ";" _sub_architecture "${_sub_architecture}")
        endif()
        string(FIND "${_line}" "==Qt=magic=Qt== Build-ABI:" _pos)
        if (_pos GREATER -1)
            math(EXPR _pos "${_pos}+26")
            string(SUBSTRING "${_line}" ${_pos} -1 _build_abi)
        endif()
    endforeach()

    if (NOT _architecture OR NOT _build_abi)
        message(FATAL_ERROR "Failed to extract architecture data from file.")
    endif()

    set(TEST_architecture 1 CACHE INTERNAL "Ran the architecture test")
    set(TEST_architecture_arch "${_architecture}" CACHE INTERNAL "Target machine architecture")
    list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_architecture_arch)
    set(TEST_subarch 1 CACHE INTERNAL "Ran machine subArchitecture test")
    foreach(it ${_sub_architecture})
        # Equivalent to qmake's QT_CPU_FEATURES.$arch.
        set(TEST_arch_${TEST_architecture_arch}_subarch_${it} 1 CACHE INTERNAL "Target sub architecture result")
        list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_arch_${TEST_architecture_arch}_subarch_${it})
    endforeach()
    set(TEST_buildAbi "${_build_abi}" CACHE INTERNAL "Target machine buildAbi")
    list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_buildAbi)
    set(QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT ${QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT} CACHE INTERNAL "Test variables that should be exported")

    list(JOIN _sub_architecture " " subarch_summary)
    set_property(GLOBAL PROPERTY qt_configure_subarch_summary "${subarch_summary}")
endfunction()


function(qt_run_linker_version_script_support)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version_flag.map" "VERS_1 { global: sym; };
VERS_2 { global: sym; }
VERS_1;
")
    if(DEFINED CMAKE_REQUIRED_FLAGS)
        set(CMAKE_REQUIRED_FLAGS_SAVE ${CMAKE_REQUIRED_FLAGS})
    else()
        set(CMAKE_REQUIRED_FLAGS "")
    endif()
    set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} "-Wl,--version-script=\"${CMAKE_CURRENT_BINARY_DIR}/version_flag.map\"")
    check_cxx_source_compiles("int main(void){return 0;}" HAVE_LD_VERSION_SCRIPT)
    if(DEFINED CMAKE_REQUIRED_FLAGS_SAVE)
        set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS_SAVE})
    endif()
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/conftest.map")

    # For some reason the linker command line written by the XCode generator, which is
    # subsequently executed by xcodebuild, ignores the linker flag, and thus the test
    # seemingly succeeds. Explicitly disable the version script test on darwin platforms.
    if(APPLE)
        set(HAVE_LD_VERSION_SCRIPT OFF)
    endif()

    set(TEST_ld_version_script "${HAVE_LD_VERSION_SCRIPT}" CACHE INTERNAL "linker version script support")
endfunction()

function(qt_run_qtbase_config_tests)
    qt_run_config_test_architecture()
    qt_run_linker_version_script_support()
endfunction()

# The qmake build of android does not perform the right architecture tests and
# forcefully disables sse4 on android x86. We have to mimic this behavior
# for now
if (CMAKE_ANDROID_ARCH_ABI STREQUAL x86)
    set(QT_FEATURE_sse4_1 OFF CACHE BOOL INTERNAL FORCE)
    set(QT_FEATURE_sse4_2 OFF CACHE BOOL INTERNAL FORCE)
    set(TEST_subarch_sse4_1 FALSE CACHE BOOL INTERNAL FORCE)
    set(TEST_subarch_sse4_2 FALSE CACHE BOOL INTERNAL FORCE)
endif()
qt_run_qtbase_config_tests()
