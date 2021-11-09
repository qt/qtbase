#
# Collection of auto detection routines to improve the user experience when
# building Qt from source.
#
# Make sure to not run detection when building standalone tests, because the detection was already
# done when initially configuring qtbase.

function(qt_internal_ensure_static_qt_config)
    if(NOT DEFINED BUILD_SHARED_LIBS)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Qt statically or dynamically" FORCE)
    endif()

    if(BUILD_SHARED_LIBS)
        message(FATAL_ERROR
            "Building Qt for ${CMAKE_SYSTEM_NAME} as shared libraries is not supported.")
    endif()
endfunction()

include("${CMAKE_CURRENT_LIST_DIR}/QtPublicWasmToolchainHelpers.cmake")
function(qt_auto_detect_wasm)
    if("${QT_QMAKE_TARGET_MKSPEC}" STREQUAL "wasm-emscripten")
        if (NOT DEFINED ENV{EMSDK})
            message(FATAL_ERROR
                "Can't find an Emscripten SDK! Make sure the EMSDK environment variable is "
                "available by activating and sourcing the emscripten sdk. Also ensure emcc is in "
                "your path.")
        endif()
        if(NOT DEFINED QT_AUTODETECT_WASM_IS_DONE)
            message(STATUS "Extracting Emscripten SDK info from EMSDK env var: $ENV{EMSDK}")
            __qt_internal_get_emroot_path_suffix_from_emsdk_env(EMROOT_PATH)

            __qt_internal_query_emsdk_version("${EMROOT_PATH}" TRUE CMAKE_EMSDK_REGEX_VERSION)
            set(EMCC_VERSION "${CMAKE_EMSDK_REGEX_VERSION}" CACHE STRING INTERNAL FORCE)

            # Find toolchain file
            if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
                __qt_internal_get_emscripten_cmake_toolchain_file_path_from_emsdk_env(
                    "${EMROOT_PATH}" wasm_toolchain_file)
                set(CMAKE_TOOLCHAIN_FILE "${wasm_toolchain_file}" CACHE STRING "" FORCE)
            endif()

            if(EXISTS "${CMAKE_TOOLCHAIN_FILE}")
                message(STATUS
                    "Emscripten ${EMCC_VERSION} toolchain file detected at ${CMAKE_TOOLCHAIN_FILE}")
            else()
                __qt_internal_show_error_no_emscripten_toolchain_file_found_when_building_qt()
            endif()

            qt_internal_ensure_static_qt_config()

            __qt_internal_get_emcc_recommended_version(recommended_version)
            set(QT_EMCC_RECOMMENDED_VERSION "${recommended_version}" CACHE STRING INTERNAL FORCE)

            set(QT_AUTODETECT_WASM_IS_DONE TRUE CACHE BOOL "")
        else()
            message(STATUS
                "Reusing cached Emscripten ${EMCC_VERSION} toolchain file detected at "
                "${CMAKE_TOOLCHAIN_FILE}")
        endif()
    endif()
endfunction()

function(qt_auto_detect_cmake_generator)
    if(NOT CMAKE_GENERATOR MATCHES "Ninja" AND NOT QT_SILENCE_CMAKE_GENERATOR_WARNING)
        message(WARNING
               "The officially supported CMake generator for building Qt is Ninja. "
               "You are using: '${CMAKE_GENERATOR}' instead. "
               "Thus, you might encounter issues. Use at your own risk.")
    endif()
endfunction()

function(qt_auto_detect_android)
    # We assume an Android build if any of the ANDROID_* cache variables are set.
    if(DEFINED ANDROID_SDK_ROOT
            OR DEFINED ANDROID_NDK_ROOT
            OR DEFINED ANDROID_ABI
            OR DEFINED ANDROID_NATIVE_ABI_LEVEL
            OR DEFINED ANDROID_STL)
        set(android_detected TRUE)
    else()
        set(android_detected FALSE)
    endif()

    # Auto-detect NDK root
    if(NOT DEFINED ANDROID_NDK_ROOT AND DEFINED ANDROID_SDK_ROOT)
        file(GLOB ndk_versions LIST_DIRECTORIES true RELATIVE "${ANDROID_SDK_ROOT}/ndk"
            "${ANDROID_SDK_ROOT}/ndk/*")
        unset(ndk_root)
        if(NOT ndk_versions STREQUAL "")
            # Use the NDK with the highest version number.
            if(CMAKE_VERSION VERSION_LESS 3.18)
                list(SORT ndk_versions)
                list(REVERSE ndk_versions)
            else()
                list(SORT ndk_versions COMPARE NATURAL ORDER DESCENDING)
            endif()
            list(GET ndk_versions 0 ndk_root)
            string(PREPEND ndk_root "${ANDROID_SDK_ROOT}/ndk/")
        else()
            # Fallback: use the deprecated "ndk-bundle" directory within the SDK root.
            set(ndk_root "${ANDROID_SDK_ROOT}/ndk-bundle")
            if(NOT IS_DIRECTORY "${ndk_root}")
                unset(ndk_root)
            endif()
        endif()
        if(DEFINED ndk_root)
            message(STATUS "Android NDK detected: ${ndk_root}")
            set(ANDROID_NDK_ROOT "${ndk_root}" CACHE STRING "")
        endif()
    endif()

    # Auto-detect toolchain file
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ANDROID_NDK_ROOT)
        set(toolchain_file "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake")
        if(EXISTS "${toolchain_file}")
            message(STATUS "Android toolchain file within NDK detected: ${toolchain_file}")
            set(CMAKE_TOOLCHAIN_FILE "${toolchain_file}" CACHE STRING "")
        else()
            message(FATAL_ERROR "Cannot find the toolchain file '${toolchain_file}'. "
                "Please specify the toolchain file with -DCMAKE_TOOLCHAIN_FILE=<file>.")
        endif()
    endif()

    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND android_detected)
        message(FATAL_ERROR "An Android build was requested, but no Android toolchain file was "
            "specified nor detected.")
    endif()

    if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT DEFINED QT_AUTODETECT_ANDROID)
        # Peek into the toolchain file and check if it looks like an Android one.
        if(NOT android_detected)
            file(READ ${CMAKE_TOOLCHAIN_FILE} toolchain_file_content OFFSET 0 LIMIT 80)
            string(FIND "${toolchain_file_content}" "The Android Open Source Project"
                find_result REVERSE)
            if(NOT ${find_result} EQUAL -1)
                set(android_detected TRUE)
            endif()
        endif()

        if(android_detected)
            message(STATUS "Android build detected, checking configuration defaults...")
            if(NOT DEFINED ANDROID_NATIVE_API_LEVEL)
                message(STATUS "ANDROID_NATIVE_API_LEVEL was not specified, using API level 23 as default")
                set(ANDROID_NATIVE_API_LEVEL 23 CACHE STRING "")
            endif()
            if(NOT DEFINED ANDROID_STL)
                set(ANDROID_STL "c++_shared" CACHE STRING "")
            endif()
        endif()
        set(QT_AUTODETECT_ANDROID ${android_detected} CACHE STRING "")
    elseif (QT_AUTODETECT_ANDROID)
        message(STATUS "Android build detected")
    endif()
endfunction()

function(qt_auto_detect_vcpkg)
    if(DEFINED ENV{VCPKG_ROOT})
        set(vcpkg_toolchain_file "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        get_filename_component(vcpkg_toolchain_file "${vcpkg_toolchain_file}" ABSOLUTE)

        if(DEFINED CMAKE_TOOLCHAIN_FILE)
            get_filename_component(supplied_toolchain_file "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
            if(NOT supplied_toolchain_file STREQUAL vcpkg_toolchain_file)
                set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}" CACHE STRING "")
            endif()
            unset(supplied_toolchain_file)
        endif()
        set(CMAKE_TOOLCHAIN_FILE "${vcpkg_toolchain_file}" CACHE STRING "" FORCE)
        message(STATUS "Using vcpkg from $ENV{VCPKG_ROOT}")
        if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
            set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
            message(STATUS "Using vcpkg triplet ${VCPKG_TARGET_TRIPLET}")
        endif()
        unset(vcpkg_toolchain_file)
        message(STATUS "CMAKE_TOOLCHAIN_FILE is: ${CMAKE_TOOLCHAIN_FILE}")
        if(DEFINED VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
            message(STATUS "VCPKG_CHAINLOAD_TOOLCHAIN_FILE is: ${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}")
        endif()
    endif()
endfunction()

function(qt_auto_detect_ios)
    if(CMAKE_SYSTEM_NAME STREQUAL iOS
            OR CMAKE_SYSTEM_NAME STREQUAL watchOS
            OR CMAKE_SYSTEM_NAME STREQUAL tvOS)
        message(STATUS "Using internal CMake ${CMAKE_SYSTEM_NAME} toolchain file.")

        # The QT_UIKIT_SDK check simulates the input.sdk condition for simulator_and_device in
        # configure.json.
        # If the variable is explicitly provided, assume simulator_and_device to be off.
        if(QT_UIKIT_SDK)
            set(simulator_and_device OFF)
        else()
            # Default to simulator_and_device when an explicit sdk is not requested.
            # Requires CMake 3.17.0+.
            set(simulator_and_device ON)
        endif()

        message(STATUS "simulator_and_device set to: \"${simulator_and_device}\".")

        # Choose relevant architectures.
        # Using a non xcode generator requires explicit setting of the
        # architectures, otherwise compilation fails with unknown defines.
        if(CMAKE_SYSTEM_NAME STREQUAL iOS)
            if(simulator_and_device)
                set(osx_architectures "arm64;x86_64")
            elseif(QT_UIKIT_SDK STREQUAL "iphoneos")
                set(osx_architectures "arm64")
            elseif(QT_UIKIT_SDK STREQUAL "iphonesimulator")
                set(osx_architectures "x86_64")
            else()
                if(NOT DEFINED QT_UIKIT_SDK)
                    message(FATAL_ERROR "Please proviude a value for -DQT_UIKIT_SDK."
                        " Possible values: iphoneos, iphonesimulator.")
                else()
                    message(FATAL_ERROR
                            "Unknown SDK argument given to QT_UIKIT_SDK: ${QT_UIKIT_SDK}.")
                endif()
            endif()
        elseif(CMAKE_SYSTEM_NAME STREQUAL tvOS)
            if(simulator_and_device)
                set(osx_architectures "arm64;x86_64")
            elseif(QT_UIKIT_SDK STREQUAL "appletvos")
                set(osx_architectures "arm64")
            elseif(QT_UIKIT_SDK STREQUAL "appletvsimulator")
                set(osx_architectures "x86_64")
            else()
                if(NOT DEFINED QT_UIKIT_SDK)
                    message(FATAL_ERROR "Please proviude a value for -DQT_UIKIT_SDK."
                        " Possible values: appletvos, appletvsimulator.")
                else()
                    message(FATAL_ERROR
                            "Unknown SDK argument given to QT_UIKIT_SDK: ${QT_UIKIT_SDK}.")
                endif()
            endif()
        elseif(CMAKE_SYSTEM_NAME STREQUAL watchOS)
            if(simulator_and_device)
                set(osx_architectures "armv7k;i386")
            elseif(QT_UIKIT_SDK STREQUAL "watchos")
                set(osx_architectures "armv7k")
            elseif(QT_UIKIT_SDK STREQUAL "watchsimulator")
                set(osx_architectures "i386")
            else()
                if(NOT DEFINED QT_UIKIT_SDK)
                    message(FATAL_ERROR "Please proviude a value for -DQT_UIKIT_SDK."
                        " Possible values: watchos, watchsimulator.")
                else()
                    message(FATAL_ERROR
                            "Unknown SDK argument given to QT_UIKIT_SDK: ${QT_UIKIT_SDK}.")
                endif()
            endif()
        endif()

        # For non simulator_and_device builds, we need to explicitly set the SYSROOT aka the sdk
        # value.
        if(QT_UIKIT_SDK)
            set(CMAKE_OSX_SYSROOT "${QT_UIKIT_SDK}" CACHE STRING "")
        endif()
        set(CMAKE_OSX_ARCHITECTURES "${osx_architectures}" CACHE STRING "")

        qt_internal_ensure_static_qt_config()

        # Disable qt rpaths for iOS, just like mkspecs/common/uikit.conf does, due to those
        # bundles not being able to use paths outside the app bundle. Not sure this is strictly
        # needed though.
        set(QT_DISABLE_RPATH "OFF" CACHE BOOL "Disable automatic Qt rpath handling." FORCE)
    endif()
endfunction()

function(qt_auto_detect_cmake_config)
    if(CMAKE_CONFIGURATION_TYPES)
        # Allow users to specify this option.
        if(NOT QT_MULTI_CONFIG_FIRST_CONFIG)
            list(GET CMAKE_CONFIGURATION_TYPES 0 first_config_type)
            set(QT_MULTI_CONFIG_FIRST_CONFIG "${first_config_type}")
            set(QT_MULTI_CONFIG_FIRST_CONFIG "${first_config_type}" PARENT_SCOPE)
        endif()

        set(CMAKE_TRY_COMPILE_CONFIGURATION "${QT_MULTI_CONFIG_FIRST_CONFIG}" PARENT_SCOPE)
        if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
            # Create build-<config>.ninja files for all specified configurations.
            set(CMAKE_CROSS_CONFIGS "all" CACHE STRING "")

            # The configuration that will be considered the main one (for example when
            # configuring standalone tests with a single-config generator like Ninja).
            set(CMAKE_DEFAULT_BUILD_TYPE "${QT_MULTI_CONFIG_FIRST_CONFIG}" CACHE STRING "")

            # By default when ninja is called without parameters, it will build all configurations.
            set(CMAKE_DEFAULT_CONFIGS "all" CACHE STRING "")
        endif()
    endif()
endfunction()

function(qt_auto_detect_cyclic_toolchain)
    if(CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "/qt.toolchain.cmake$")
        message(FATAL_ERROR
                "Woah there! You can't use the Qt generated qt.toolchain.cmake file to configure "
                "qtbase, because that will create a toolchain file that includes itself!\n"
                "Did you accidentally use qt-cmake to configure qtbase? Make sure to remove the "
                "CMakeCache.txt file, and configure qtbase with 'cmake' instead of 'qt-cmake'.")
    endif()
endfunction()

function(qt_internal_get_darwin_sdk_version out_var)
    if(APPLE)
        if(IOS)
            set(sdk_name "iphoneos")
        elseif(TVOS)
            set(sdk_name "appletvos")
        elseif(WATCHOS)
            set(sdk_name "watchos")
        else()
            # Default to macOS
            set(sdk_name "macosx")
        endif()
        set(xcrun_version_arg "--show-sdk-version")
        execute_process(COMMAND /usr/bin/xcrun --sdk ${sdk_name} ${xcrun_version_arg}
                        OUTPUT_VARIABLE sdk_version
                        ERROR_VARIABLE xcrun_error)
        if(NOT sdk_version)
            message(FATAL_ERROR
                    "Can't determine darwin ${sdk_name} SDK version. Error: ${xcrun_error}")
        endif()
        string(STRIP "${sdk_version}" sdk_version)
        set(${out_var} "${sdk_version}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_internal_get_xcode_version out_var)
    if(APPLE)
        execute_process(COMMAND /usr/bin/xcrun  xcodebuild -version
                        OUTPUT_VARIABLE xcode_version
                        ERROR_VARIABLE xcrun_error)
        string(REPLACE "\n" " " xcode_version "${xcode_version}")
        string(STRIP "${xcode_version}" xcode_version)
        set(${out_var} "${xcode_version}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_auto_detect_darwin)
    if(APPLE)
        # If no CMAKE_OSX_DEPLOYMENT_TARGET is provided, default to a value that Qt defines.
        # This replicates the behavior in mkspecs/common/macx.conf where
        # QMAKE_MACOSX_DEPLOYMENT_TARGET is set.
        set(description
            "Minimum OS X version to target for deployment (at runtime); newer APIs weak linked. Set to empty string for default value.")
        if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
            if(NOT CMAKE_SYSTEM_NAME)
                # macOS
                set(version "10.14")
            elseif(CMAKE_SYSTEM_NAME STREQUAL iOS)
                set(version "13.0")
            elseif(CMAKE_SYSTEM_NAME STREQUAL watchOS)
                set(version "6.0")
            elseif(CMAKE_SYSTEM_NAME STREQUAL tvOS)
                set(version "13.0")
            endif()
            if(version)
                set(CMAKE_OSX_DEPLOYMENT_TARGET "${version}" CACHE STRING "${description}")
            endif()
        endif()

        qt_internal_get_darwin_sdk_version(darwin_sdk_version)
        set(QT_MAC_SDK_VERSION "${darwin_sdk_version}" CACHE STRING "Darwin SDK version.")

        qt_internal_get_xcode_version(xcode_version)
        set(QT_MAC_XCODE_VERSION "${xcode_version}" CACHE STRING "Xcode version.")

        set(device_names "iOS" "watchOS" "tvOS")
        list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)
        if(NOT CMAKE_SYSTEM_NAME IN_LIST device_names AND arch_count GREATER 0)
            foreach(arch ${CMAKE_OSX_ARCHITECTURES})
                if(arch STREQUAL "arm64e")
                    message(WARNING "Applications built against an arm64e Qt architecture will "
                                     "likely fail to run on Apple Silicon. Consider targeting "
                                     "'arm64' instead.")
                endif()
            endforeach()
        endif()
    endif()
endfunction()

function(qt_auto_detect_macos_universal)
    set(device_names "iOS" "watchOS" "tvOS")
    if(APPLE AND NOT CMAKE_SYSTEM_NAME IN_LIST device_names)
        list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)

        set(is_universal "OFF")
        if(arch_count GREATER 1)
            set(is_universal "ON")
        endif()

        set(QT_IS_MACOS_UNIVERSAL "${is_universal}" CACHE INTERNAL "Build universal Qt for macOS")
    endif()
endfunction()

function(qt_auto_detect_pch)
    set(default_value "ON")

    if(CMAKE_OSX_ARCHITECTURES AND CMAKE_VERSION VERSION_LESS 3.18.0 AND NOT QT_FORCE_PCH)
        list(LENGTH CMAKE_OSX_ARCHITECTURES arch_count)
        # CMake versions lower than 3.18 don't support PCH when multiple architectures are set.
        # This is the case for simulator_and_device builds.
        if(arch_count GREATER 1)
            set(default_value "OFF")
            message(WARNING "PCH support disabled due to usage of multiple architectures.")
        endif()
    endif()

    option(BUILD_WITH_PCH "Build Qt using precompiled headers?" "${default_value}")
endfunction()

function(qt_auto_detect_win32_arm)
    if("${QT_QMAKE_TARGET_MKSPEC}" STREQUAL "win32-arm64-msvc")
        set(CMAKE_SYSTEM_NAME "Windows" CACHE STRING "")
        set(CMAKE_SYSTEM_VERSION "10" CACHE STRING "")
        set(CMAKE_SYSTEM_PROCESSOR "arm64" CACHE STRING "")
    endif()
endfunction()

function(qt_auto_detect_integrity)
    if(
       # Qt's custom CMake toolchain file sets this value.
       CMAKE_SYSTEM_NAME STREQUAL "Integrity" OR

       # Upstream CMake expects this name, but we don't currently use it in Qt.
       CMAKE_SYSTEM_NAME STREQUAL "GHS-MULTI"
    )
        qt_internal_ensure_static_qt_config()
    endif()
endfunction()

# Let CMake load our custom platform modules.
# CMake-provided platform modules take precedence.
if(NOT QT_AVOID_CUSTOM_PLATFORM_MODULES)
    list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/platforms")
endif()

qt_auto_detect_cmake_generator()
qt_auto_detect_cyclic_toolchain()
qt_auto_detect_cmake_config()
qt_auto_detect_darwin()
qt_auto_detect_macos_universal()
qt_auto_detect_ios()
qt_auto_detect_android()
qt_auto_detect_vcpkg()
qt_auto_detect_pch()
qt_auto_detect_wasm()
qt_auto_detect_win32_arm()
qt_auto_detect_integrity()
