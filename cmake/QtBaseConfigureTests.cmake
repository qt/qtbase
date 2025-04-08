# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(CheckCXXSourceCompiles)

function(qt_internal_run_config_test_architecture)
    set(no_value_options "")
    set(single_value_options
        FLAVOR
        OUT_VAR_ARCH
        OUT_VAR_ABI
        OUT_VAR_SUBARCH
    )
    set(multi_value_options
        CMAKE_FLAGS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )

    set(flags "${arg_CMAKE_FLAGS}")
    list(TRANSFORM flags PREPEND "            " OUTPUT_VARIABLE flags_indented)
    list(JOIN flags_indented "\n" flags_indented)

    set(project_label "")
    set(binary_dir_suffix "")
    if(DEFINED arg_FLAVOR)
        set(project_label " (${arg_FLAVOR})")
        set(binary_dir_suffix "-${arg_FLAVOR}")
    endif()

    message(STATUS
            "Building architecture extraction project${project_label} with the following CMake arguments:")
    list(POP_BACK CMAKE_MESSAGE_CONTEXT _context)
    message(STATUS ${flags_indented})
    list(APPEND CMAKE_MESSAGE_CONTEXT ${_context})

    try_compile(
        _arch_result
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/arch${binary_dir_suffix}"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/arch"
        arch
        CMAKE_FLAGS ${flags}
        OUTPUT_VARIABLE arch_test_output
        )

    if (NOT _arch_result)
        message(FATAL_ERROR
                "Failed to build architecture extraction project. Build output:\n ${arch_test_output}")
    endif()

    set(_arch_file_suffix "${CMAKE_EXECUTABLE_SUFFIX}")
    # With emscripten the application entry point is a .js file (to be run with node for example),
    # but the real "data" is in the .wasm file, so that's where we need to look for the ABI, etc.
    # information.
    if (WASM)
        set(_arch_file_suffix ".wasm")
    endif()

    set(arch_test_location "config.tests/arch${binary_dir_suffix}")
    if(QT_MULTI_CONFIG_FIRST_CONFIG)
        string(APPEND arch_test_location "/${QT_MULTI_CONFIG_FIRST_CONFIG}")
    endif()

    set(arch_dir "${CMAKE_CURRENT_BINARY_DIR}/${arch_test_location}")
    file(GLOB arch_dir_globbed_files RELATIVE "${arch_dir}" "${arch_dir}/*")
    list(JOIN arch_dir_globbed_files "\n" arch_dir_globbed_files)

    set(_arch_file
        "${CMAKE_CURRENT_BINARY_DIR}/${arch_test_location}/architecture_test${_arch_file_suffix}")
    if (NOT EXISTS "${_arch_file}")
        message(FATAL_ERROR
                "Failed to find compiled architecture detection executable at ${_arch_file}. \
                The following files were found at: ${arch_dir} \
                ${arch_dir_globbed_files}")
    endif()
    message(STATUS "Extracting architecture info from ${_arch_file}.")

    cmake_policy(PUSH)
    if(POLICY CMP0159)
        cmake_policy(SET CMP0159 NEW)
    endif()
    file(STRINGS "${_arch_file}" _arch_lines LENGTH_MINIMUM 16 LENGTH_MAXIMUM 1024 ENCODING UTF-8
         REGEX "==Qt=magic=Qt==")
    cmake_policy(POP)

    set(architectures "")
    foreach (_line ${_arch_lines})
        string(LENGTH "${_line}" lineLength)
        string(FIND "${_line}" "==Qt=magic=Qt== Architecture:" _pos)
        if (_pos GREATER -1)
            math(EXPR _pos "${_pos}+29")
            string(SUBSTRING "${_line}" ${_pos} -1 _architecture)
        endif()
        string(FIND "${_line}" "==Qt=magic=Qt== Sub-architecture:" _pos)
        if (_pos GREATER -1 AND NOT _line MATCHES "Sub-architecture:$")
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
        list(SUBLIST _arch_lines 0 5 arch_lines_fewer)
        list(JOIN arch_lines_fewer "\n" arch_lines_output)

        message(FATAL_ERROR
                "Failed to extract architecture data from file. \
                Here are the first few lines extracted:\n${arch_lines_output}")
    endif()

    set("${arg_OUT_VAR_ARCH}" "${_architecture}" PARENT_SCOPE)
    set("${arg_OUT_VAR_SUBARCH}" "${_sub_architecture}" PARENT_SCOPE)
    set("${arg_OUT_VAR_ABI}" "${_build_abi}" PARENT_SCOPE)
endfunction()

function(qt_run_config_test_architecture)
    set(QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT
        "" CACHE INTERNAL "Test variables that should be exported" FORCE)

    # Compile test to find the target architecture and sub-architectures.
    qt_get_platform_try_compile_vars(platform_try_compile_vars)
    list(APPEND flags ${platform_try_compile_vars})

    set(first_arch "")
    set(architectures "")
    if("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
        qt_internal_run_config_test_architecture(
            CMAKE_FLAGS ${flags}
            OUT_VAR_ARCH arch
            OUT_VAR_SUBARCH subarch
            OUT_VAR_ABI abi
        )
        set(first_arch "${arch}")
        set(architectures "${arch}")
        set(sub_architecture_${arch} "${subarch}")
        set(build_abi_${arch} "${abi}")
    else()
        list(FILTER flags EXCLUDE REGEX "^-DCMAKE_OSX_ARCHITECTURES[:=]")
        foreach(osxarch IN LISTS CMAKE_OSX_ARCHITECTURES)
            set(local_flags ${flags})
            if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
                if(osxarch STREQUAL "x86_64")
                    list(APPEND local_flags "-DCMAKE_OSX_SYSROOT=iphonesimulator")
                endif()
            endif()
            qt_internal_run_config_test_architecture(
                FLAVOR "${osxarch}"
                CMAKE_FLAGS ${local_flags} -DCMAKE_OSX_ARCHITECTURES=${osxarch}
                OUT_VAR_ARCH arch
                OUT_VAR_SUBARCH subarch
                OUT_VAR_ABI abi
            )
            if(first_arch STREQUAL "")
                set(first_arch "${arch}")
            endif()
            list(APPEND architectures "${arch}")
            set(sub_architecture_${arch} "${subarch}")
            set(build_abi_${arch} "${abi}")
        endforeach()
    endif()

    set(TEST_architecture 1 CACHE INTERNAL "Ran the architecture test")
    list(GET architectures 0 first_arch)
    set(TEST_architecture_arch "${first_arch}" CACHE INTERNAL "Target machine architecture")
    list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_architecture_arch)
    set(TEST_architecture_architectures "${architectures}" CACHE INTERNAL "Target machine architectures")
    list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_architecture_architectures)
    set(TEST_subarch 1 CACHE INTERNAL "Ran machine subArchitecture test")
    set(TEST_subarch_result "${sub_architecture_${first_arch}}" CACHE INTERNAL "Target sub-architectures")
    list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_subarch_result)
    foreach(arch IN LISTS architectures)
        # Extended version of qmake's QT_CPU_FEATURES.$arch.
        set(TEST_arch_${arch}_abi "${build_abi_${arch}}" CACHE INTERNAL "Target architecture ABI")
        list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_arch_${arch}_abi)
        foreach(it IN LISTS sub_architecture_${arch})
            set(TEST_arch_${arch}_subarch_${it} 1 CACHE INTERNAL "Target sub architecture result")
            list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_arch_${arch}_subarch_${it})
        endforeach()
    endforeach()
    set(TEST_buildAbi "${_build_abi}" CACHE INTERNAL "Target machine buildAbi")
    list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_buildAbi)
    set(QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT ${QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT} CACHE INTERNAL "Test variables that should be exported")

    list(JOIN sub_architecture_${first_arch} " " subarch_summary)
    set_property(GLOBAL PROPERTY qt_configure_subarch_summary "${subarch_summary}")
endfunction()


function(qt_run_linker_version_script_support)
    # For some reason the linker command line written by the XCode generator, which is
    # subsequently executed by xcodebuild, ignores the linker flag, and thus the test
    # seemingly succeeds. Explicitly disable the version script test on darwin platforms.
    # Also makes no sense with MSVC-style command-line
    if(NOT APPLE AND NOT MSVC)
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/version_flag.map" [=[
            VERS_1 { global: sym1; };
            VERS_2 { global: sym2; } VERS_1;
        ]=])
        set(CMAKE_REQUIRED_LINK_OPTIONS "")
        list(APPEND CMAKE_REQUIRED_LINK_OPTIONS
             "-Wl,--version-script=${CMAKE_CURRENT_BINARY_DIR}/version_flag.map")
        # Pass the linker that the main project uses to the version script compile test.
        qt_internal_get_active_linker_flags(linker_flags)
        if(linker_flags)
            list(APPEND CMAKE_REQUIRED_LINK_OPTIONS ${linker_flags})
        endif()
        check_cxx_source_compiles([=[
            int sym1;
            int sym2;
            int main(void) { return 0; }
        ]=] HAVE_LD_VERSION_SCRIPT)
        file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/version_flag.map")
    else()
        set(HAVE_LD_VERSION_SCRIPT OFF)
    endif()

    set(TEST_ld_version_script "${HAVE_LD_VERSION_SCRIPT}"
        CACHE INTERNAL "linker version script support")
    list(APPEND QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT TEST_ld_version_script)
    set(QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT ${QT_BASE_CONFIGURE_TESTS_VARS_TO_EXPORT}
        CACHE INTERNAL "Test variables that should be exported")
endfunction()

function(qt_internal_ensure_latest_win_nt_api)
    if(NOT WIN32)
        return()
    endif()
    check_cxx_source_compiles([=[
        #include <windows.h>
        #if !defined(_WIN32_WINNT) && !defined(WINVER)
        #error "_WIN32_WINNT and WINVER are not defined"
        #endif
        #if defined(_WIN32_WINNT) && (_WIN32_WINNT < 0x0A00)
        #error "_WIN32_WINNT version too low"
        #endif
        #if defined(WINVER) && (WINVER < 0x0A00)
        #error "WINVER version too low"
        #endif
        int main() { return 0; }
    ]=] HAVE_WIN10_WIN32_WINNT)
    if(NOT HAVE_WIN10_WIN32_WINNT)
        list(APPEND QT_PLATFORM_DEFINITIONS _WIN32_WINNT=0x0A00 WINVER=0x0A00)
        set(QT_PLATFORM_DEFINITIONS ${QT_PLATFORM_DEFINITIONS}
            CACHE STRING "Qt platform specific pre-processor defines" FORCE)
    endif()
endfunction()

function(qt_run_qtbase_config_tests)
    qt_run_config_test_architecture()
    qt_internal_ensure_latest_win_nt_api()
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

function(qt_internal_print_cmake_darwin_info)
    if(APPLE)
        if(NOT CMAKE_OSX_ARCHITECTURES)
            set(default_osx_arch " (defaults to ${CMAKE_SYSTEM_PROCESSOR})")
        endif()
        message(STATUS "CMAKE_OSX_ARCHITECTURES: \"${CMAKE_OSX_ARCHITECTURES}\"${default_osx_arch}")
        message(STATUS "CMAKE_OSX_SYSROOT: \"$CACHE{CMAKE_OSX_SYSROOT}\" / \"${CMAKE_OSX_SYSROOT}\"")
        message(STATUS "QT_APPLE_SDK_PATH: \"${QT_APPLE_SDK_PATH}\"")
        message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET: \"${CMAKE_OSX_DEPLOYMENT_TARGET}\"")
        message(STATUS "QT_MAC_SDK_VERSION: \"${QT_MAC_SDK_VERSION}\"")
        message(STATUS "QT_MAC_XCODE_VERSION: \"${QT_MAC_XCODE_VERSION}\"")

        if(DEFINED CACHE{QT_IS_MACOS_UNIVERSAL})
            message(STATUS "QT_IS_MACOS_UNIVERSAL: \"${QT_IS_MACOS_UNIVERSAL}\"")
        endif()
        if(QT_APPLE_SDK)
            message(STATUS "QT_APPLE_SDK: \"${QT_APPLE_SDK}\"")
        endif()
        qt_internal_get_first_osx_arch(osx_first_arch)
        if(osx_first_arch)
            message(STATUS "Configure tests main architecture (in multi-arch build): \"${osx_first_arch}\"")
        endif()
    endif()
endfunction()
qt_internal_print_cmake_darwin_info()

function(qt_internal_print_cmake_host_and_target_info)
    message(STATUS "CMAKE_VERSION: \"${CMAKE_VERSION}\"")
    message(STATUS "CMAKE_HOST_SYSTEM: \"${CMAKE_HOST_SYSTEM}\"")
    message(STATUS "CMAKE_HOST_SYSTEM_NAME: \"${CMAKE_HOST_SYSTEM_NAME}\"")
    message(STATUS "CMAKE_HOST_SYSTEM_VERSION: \"${CMAKE_HOST_SYSTEM_VERSION}\"")
    message(STATUS "CMAKE_HOST_SYSTEM_PROCESSOR: \"${CMAKE_HOST_SYSTEM_PROCESSOR}\"")

    message(STATUS "CMAKE_SYSTEM: \"${CMAKE_SYSTEM_NAME}\"")
    message(STATUS "CMAKE_SYSTEM_NAME: \"${CMAKE_SYSTEM_NAME}\"")
    message(STATUS "CMAKE_SYSTEM_VERSION: \"${CMAKE_SYSTEM_VERSION}\"")
    message(STATUS "CMAKE_SYSTEM_PROCESSOR: \"${CMAKE_SYSTEM_PROCESSOR}\"")

    message(STATUS "CMAKE_CROSSCOMPILING: \"${CMAKE_CROSSCOMPILING}\"")
endfunction()
qt_internal_print_cmake_host_and_target_info()

function(qt_internal_print_prefix_info)
    message(STATUS "CMAKE_INSTALL_PREFIX: \"${CMAKE_INSTALL_PREFIX}\"")
    message(STATUS "CMAKE_STAGING_PREFIX: \"${CMAKE_STAGING_PREFIX}\"")
    message(STATUS "QT_BUILD_DIR: \"${QT_BUILD_DIR}\"")
    message(STATUS "QT_INSTALL_DIR: \"${QT_INSTALL_DIR}\"")
    message(STATUS "QT_WILL_INSTALL: \"${QT_WILL_INSTALL}\"")
endfunction()
qt_internal_print_prefix_info()

function(qt_internal_print_cmake_compiler_info)
    message(STATUS "CMAKE_C_COMPILER: \"${CMAKE_C_COMPILER}\" (${CMAKE_C_COMPILER_VERSION})")
    message(STATUS "CMAKE_CXX_COMPILER: \"${CMAKE_CXX_COMPILER}\" (${CMAKE_CXX_COMPILER_VERSION})")
    if(CMAKE_OBJC_COMPILER)
        message(STATUS "CMAKE_OBJC_COMPILER: \"${CMAKE_OBJC_COMPILER}\" (${CMAKE_OBJC_COMPILER_VERSION})")
    endif()
    if(CMAKE_OBJCXX_COMPILER)
        message(STATUS "CMAKE_OBJCXX_COMPILER: \"${CMAKE_OBJCXX_COMPILER}\" (${CMAKE_OBJCXX_COMPILER_VERSION})")
    endif()
endfunction()
qt_internal_print_cmake_compiler_info()

function(qt_internal_print_cmake_windows_info)
    if(MSVC_VERSION)
        message(STATUS "MSVC_VERSION: \"${MSVC_VERSION}\"")
    endif()
    if(MSVC_TOOLSET_VERSION)
        message(STATUS "MSVC_TOOLSET_VERSION: \"${MSVC_TOOLSET_VERSION}\"")
    endif()
endfunction()
qt_internal_print_cmake_windows_info()

function(qt_internal_print_cmake_android_info)
    if(ANDROID)
        message(STATUS "ANDROID_TOOLCHAIN: \"${ANDROID_TOOLCHAIN}\"")
        message(STATUS "ANDROID_NDK: \"${ANDROID_NDK}\"")
        message(STATUS "ANDROID_ABI: \"${ANDROID_ABI}\"")
        message(STATUS "ANDROID_PLATFORM: \"${ANDROID_PLATFORM}\"")
        message(STATUS "ANDROID_NATIVE_API_LEVEL: \"${ANDROID_NATIVE_API_LEVEL}\"")
        message(STATUS "ANDROID_STL: \"${ANDROID_STL}\"")
        message(STATUS "ANDROID_PIE: \"${ANDROID_PIE}\"")
        message(STATUS "ANDROID_CPP_FEATURES: \"${ANDROID_CPP_FEATURES}\"")
        message(STATUS "ANDROID_ALLOW_UNDEFINED_SYMBOLS: \"${ANDROID_ALLOW_UNDEFINED_SYMBOLS}\"")
        message(STATUS "ANDROID_ARM_MODE: \"${ANDROID_ARM_MODE}\"")
        message(STATUS "ANDROID_ARM_NEON: \"${ANDROID_ARM_NEON}\"")
        message(STATUS "ANDROID_DISABLE_FORMAT_STRING_CHECKS: \"${ANDROID_DISABLE_FORMAT_STRING_CHECKS}\"")
        message(STATUS "ANDROID_LLVM_TRIPLE: \"${ANDROID_LLVM_TRIPLE}\"")
    endif()
endfunction()
qt_internal_print_cmake_android_info()
