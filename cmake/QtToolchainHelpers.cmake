# Create a CMake toolchain file for convenient configuration of both internal Qt builds
# as well as CMake application projects.
# Expects various global variables to be set.
function(qt_internal_create_toolchain_file)
    set(qt_host_path_required FALSE)

    if(NOT "${QT_HOST_PATH}" STREQUAL "")
        # If a QT_HOST_PATH is provided when configuring qtbase, we assume it's a cross build
        # and thus we require the QT_HOST_PATH to be provided also when using the cross-built Qt.
        # This tells the Qt toolchain file to do appropriate requirement checks.
        set(qt_host_path_required TRUE)

        # TODO: Figure out how to make the initial QT_HOST_PATH var relocatable in relation
        # to the target CMAKE_INSTALL_DIR, if at all possible to do so in a reliable way.
        get_filename_component(qt_host_path_absolute "${QT_HOST_PATH}" ABSOLUTE)
        get_filename_component(qt_host_path_cmake_dir_absolute
            "${Qt${PROJECT_VERSION_MAJOR}HostInfo_DIR}/.." ABSOLUTE)
    endif()

    if(CMAKE_TOOLCHAIN_FILE)
        file(TO_CMAKE_PATH "${CMAKE_TOOLCHAIN_FILE}" __qt_chainload_toolchain_file)
        set(init_original_toolchain_file
            "
set(__qt_initially_configured_toolchain_file \"${__qt_chainload_toolchain_file}\")
set(__qt_chainload_toolchain_file \"\${__qt_initially_configured_toolchain_file}\")
")
    endif()

    if(VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        file(TO_CMAKE_PATH "${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}" VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        list(APPEND init_vcpkg
             "set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE \"${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}\")")
    endif()

    if(VCPKG_TARGET_TRIPLET)
        list(APPEND init_vcpkg
             "set(VCPKG_TARGET_TRIPLET \"${VCPKG_TARGET_TRIPLET}\" CACHE STRING \"\")")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Windows" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" AND CMAKE_SYSTEM_VERSION STREQUAL "10")
        list(APPEND init_platform "set(CMAKE_SYSTEM_NAME Windows CACHE STRING \"\")")
        list(APPEND init_platform "set(CMAKE_SYSTEM_VERSION 10 CACHE STRING \"\")")
        list(APPEND init_platform "set(CMAKE_SYSTEM_PROCESSOR arm64 CACHE STRING \"\")")
    endif()

    # By default we don't want to allow mixing compilers for building different repositories, so we
    # embed the initially chosen compilers into the toolchain.
    # This is because on Windows compilers aren't easily mixed.
    # We want to avoid that qtbase is built using cl.exe for example, and then for another repo
    # gcc is picked up from %PATH%.
    # The same goes when using a custom compiler on other platforms, such as ICC.
    #
    # There are a few exceptions though.
    #
    # When crosscompiling using Boot2Qt, the environment setup shell script sets up the CXX env var,
    # which is used by CMake to determine the initial compiler that should be used.
    # Unfortunately, the CXX env var contains not only the compiler name, but also a few required
    # arch-specific compiler flags. This means that when building qtsvg, if the Qt created toolchain
    # file sets the CMAKE_CXX_COMPILER variable, the CXX env var is ignored and thus the extra
    # arch specific compiler flags are not picked up anymore, leading to a configuration failure.
    #
    # To avoid this issue, disable automatic embedding of the compilers into the qt toolchain when
    # cross compiling. This is merely a heuristic, becacuse we don't have enough data to decide
    # when to do it or not.
    # For example on Linux one might want to allow mixing of clang and gcc (maybe).
    #
    # To allow such use cases when the default is wrong, one can provide a flag to explicitly opt-in
    # or opt-out of the compiler embedding into the Qt toolchain.
    #
    # Passing -DQT_EMBED_TOOLCHAIN_COMPILER=ON  will force embedding of the compilers.
    # Passing -DQT_EMBED_TOOLCHAIN_COMPILER=OFF will disable embedding of the compilers.
    set(__qt_embed_toolchain_compilers TRUE)
    if(CMAKE_CROSSCOMPILING)
        set(__qt_embed_toolchain_compilers FALSE)
    endif()
    if(DEFINED QT_EMBED_TOOLCHAIN_COMPILER)
        if(QT_EMBED_TOOLCHAIN_COMPILER)
            set(__qt_embed_toolchain_compilers TRUE)
        else()
            set(__qt_embed_toolchain_compilers FALSE)
        endif()
    endif()
    if(__qt_embed_toolchain_compilers)
        list(APPEND init_platform "
    set(__qt_initial_c_compiler \"${CMAKE_C_COMPILER}\")
    set(__qt_initial_cxx_compiler \"${CMAKE_CXX_COMPILER}\")
    if(NOT DEFINED CMAKE_C_COMPILER AND EXISTS \"\${__qt_initial_c_compiler}\")
        set(CMAKE_C_COMPILER \"\${__qt_initial_c_compiler}\" CACHE STRING \"\")
    endif()
    if(NOT DEFINED CMAKE_CXX_COMPILER AND EXISTS \"\${__qt_initial_cxx_compiler}\")
        set(CMAKE_CXX_COMPILER \"\${__qt_initial_cxx_compiler}\" CACHE STRING \"\")
    endif()")
    endif()

    unset(init_additional_used_variables)
    if(APPLE)

        # For an iOS simulator_and_device build, we should not explicitly set the sysroot, but let
        # CMake do it's universal build magic to use one sysroot / sdk per-arch.
        # For a single arch / sysroot iOS build, try to use the initially configured sysroot
        # path if it exists, otherwise just set the name of the sdk to be used.
        # The latter "name" part is important for user projects so that running 'xcodebuild' from
        # the command line chooses the correct sdk.
        # Also allow to opt out just in case.
        #
        # TODO: Figure out if the same should apply to universal macOS builds.

        list(LENGTH CMAKE_OSX_ARCHITECTURES _qt_osx_architectures_count)
        if(CMAKE_OSX_SYSROOT AND NOT _qt_osx_architectures_count GREATER 1 AND UIKIT)
            list(APPEND init_platform "
    set(__qt_uikit_sdk \"${QT_UIKIT_SDK}\")
    set(__qt_initial_cmake_osx_sysroot \"${CMAKE_OSX_SYSROOT}\")
    if(NOT DEFINED CMAKE_OSX_SYSROOT AND EXISTS \"\${__qt_initial_cmake_osx_sysroot}\")
        set(CMAKE_OSX_SYSROOT \"\${__qt_initial_cmake_osx_sysroot}\" CACHE PATH \"\")
    elseif(NOT DEFINED CMAKE_OSX_SYSROOT AND NOT QT_NO_SET_OSX_SYSROOT)
        set(CMAKE_OSX_SYSROOT \"\${__qt_uikit_sdk}\" CACHE PATH \"\")
    endif()")
        endif()

        if(CMAKE_OSX_DEPLOYMENT_TARGET)
            list(APPEND init_platform
                "set(CMAKE_OSX_DEPLOYMENT_TARGET \"${CMAKE_OSX_DEPLOYMENT_TARGET}\" CACHE STRING \"\")")
        endif()

        # Save list of initial architectures Qt was configured with.
        set(_qt_osx_architectures_escaped "${CMAKE_OSX_ARCHITECTURES}")
        string(REPLACE ";" "LITERAL_SEMICOLON"
            _qt_osx_architectures_escaped "${_qt_osx_architectures_escaped}")
        set(docstring "List of architectures Qt was built with")
        list(APPEND init_platform
            "set(QT_OSX_ARCHITECTURES \"${_qt_osx_architectures_escaped}\" CACHE STRING \"${docstring}\")")
        list(APPEND init_platform "")

        # When building another qt repo, ensure the same list of architectures is used by default.
        # Detection of a qt repo is done by checking for QT_REPO_MODULE_VERSION which is set in
        # the repo's .cmake.conf file.
        # Most standalone tests will also be built with multiple architectures.
        # Certain tests will be built with a single arch only (like tests/auto/cmake) to avoid
        # issues in the CI when trying to build them on VMs that do not have a universal macOS
        # SDK.
        list(APPEND init_platform
            "# Only build multiple architectures when building Qt itself. Can be explicitly enabled or disabled.")
        list(APPEND init_platform "if((QT_REPO_MODULE_VERSION AND NOT QT_FORCE_SINGLE_QT_OSX_ARCHITECTURE) OR QT_FORCE_ALL_QT_OSX_ARCHITECTURES)")
        list(APPEND init_platform "    set(__qt_toolchain_building_qt_repo TRUE)")
        list(APPEND init_platform "    set(CMAKE_OSX_ARCHITECTURES \"\${QT_OSX_ARCHITECTURES}\" CACHE STRING \"\")")
        list(APPEND init_platform "endif()")
        list(APPEND init_platform "")

        # For macOS user projects, default to not specifying any architecture. This means CMake will
        # not pass an -arch flag to the compiler and the compiler will choose the default
        # architecture to build for.
        # On Apple Silicon, CMake will introspect whether it's running under Rosetta and will
        # pass the detected architecture (x86_64 under Rosetta or arm64 natively) to the compiler.
        # This is line with default CMake behavior for user projects.
        #
        # For iOS, we provide a bit more convenience.
        # When the user project is built using the Xcode generator, we only specify the architecture
        # if this is a single architecture Qt for iOS build. If we wouldn't, invoking just
        # xcodebuild from the command line would try to build with the wrong architecture. Also
        # provide an opt-out option just in case.
        #
        # For a multi-architecture build (so simulator_and_device) we don't set an explicit
        # architecture and let Xcode and the developer handle it.
        #
        # When using the Ninja generator, specify the first architecture from QT_OSX_ARCHITECTURES
        # (even with a simulator_and_device Qt build). This ensures that the default configuration
        # at least tries to build something.
        if(UIKIT)
            qt_internal_get_first_osx_arch(osx_first_arch)
            list(APPEND init_platform
"if((NOT CMAKE_GENERATOR STREQUAL \"Xcode\" AND NOT __qt_toolchain_building_qt_repo)
    OR (CMAKE_GENERATOR STREQUAL \"Xcode\" AND __qt_uikit_sdk AND NOT QT_NO_SET_OSX_ARCHITECTURES))")
            list(APPEND init_platform
                "    set(CMAKE_OSX_ARCHITECTURES \"${osx_first_arch}\" CACHE STRING \"\")")
            list(APPEND init_platform "endif()")
            list(APPEND init_platform "")
        endif()

        if(UIKIT)
            list(APPEND init_platform
                "set(CMAKE_SYSTEM_NAME \"${CMAKE_SYSTEM_NAME}\" CACHE STRING \"\")")
            list(APPEND init_platform "if(CMAKE_GENERATOR STREQUAL \"Xcode\" AND NOT QT_NO_XCODE_EMIT_EPN)")
            list(APPEND init_platform "    set_property(GLOBAL PROPERTY XCODE_EMIT_EFFECTIVE_PLATFORM_NAME OFF)")
            list(APPEND init_platform "endif()")
        endif()
    elseif(ANDROID)
        foreach(var ANDROID_PLATFORM ANDROID_NATIVE_API_LEVEL ANDROID_STL
                ANDROID_ABI ANDROID_SDK_ROOT ANDROID_NDK_ROOT)
            list(APPEND init_additional_used_variables
                "list(APPEND __qt_toolchain_used_variables ${var})")
        endforeach()

        list(APPEND init_platform
            "if(NOT DEFINED ANDROID_PLATFORM AND NOT DEFINED ANDROID_NATIVE_API_LEVEL)")
        list(APPEND init_platform
            "    set(ANDROID_PLATFORM \"${ANDROID_PLATFORM}\" CACHE STRING \"\")")
        list(APPEND init_platform "endif()")
        list(APPEND init_platform "set(ANDROID_STL \"${ANDROID_STL}\" CACHE STRING \"\")")
        list(APPEND init_platform "set(ANDROID_ABI \"${ANDROID_ABI}\" CACHE STRING \"\")")
        list(APPEND init_platform "if (NOT DEFINED ANDROID_SDK_ROOT)")
        file(TO_CMAKE_PATH "${ANDROID_SDK_ROOT}" __qt_android_sdk_root)
        list(APPEND init_platform
             "    set(ANDROID_SDK_ROOT \"${__qt_android_sdk_root}\" CACHE STRING \"\")")
        list(APPEND init_platform "endif()")

        list(APPEND init_platform "if(NOT \"$\{ANDROID_NDK_ROOT\}\" STREQUAL \"\")")
        list(APPEND init_platform
            "    set(__qt_toolchain_file_candidate \"$\{ANDROID_NDK_ROOT\}/build/cmake/android.toolchain.cmake\")")
        list(APPEND init_platform "    if(EXISTS \"$\{__qt_toolchain_file_candidate\}\")")
        list(APPEND init_platform
            "        message(STATUS \"Android toolchain file within NDK detected: $\{__qt_toolchain_file_candidate\}\")")
        list(APPEND init_platform "        set(__qt_chainload_toolchain_file \"$\{__qt_toolchain_file_candidate\}\")")
        list(APPEND init_platform "    else()")
        list(APPEND init_platform
            "        message(FATAL_ERROR \"Cannot find the toolchain file '$\{__qt_toolchain_file_candidate\}'. \"")
        list(APPEND init_platform
            "            \"Please specify the toolchain file with -DQT_CHAINLOAD_TOOLCHAIN_FILE=<file>.\")")
        list(APPEND init_platform "    endif()")
        list(APPEND init_platform "endif()")
    elseif(EMSCRIPTEN)
        list(APPEND init_platform
"include(\${CMAKE_CURRENT_LIST_DIR}/QtPublicWasmToolchainHelpers.cmake)
if(DEFINED ENV{EMSDK} AND NOT \"\$ENV{EMSDK}\" STREQUAL \"\")
    __qt_internal_get_emroot_path_suffix_from_emsdk_env(__qt_toolchain_emroot_path)
    __qt_internal_get_emscripten_cmake_toolchain_file_path_from_emsdk_env(
        \"\${__qt_toolchain_emroot_path}\" _qt_candidate_emscripten_toolchain_path)
    set(__qt_chainload_toolchain_file \"\${_qt_candidate_emscripten_toolchain_path}\")
endif()
")
        list(APPEND init_post_chainload_toolchain "
if(NOT __qt_chainload_toolchain_file_included)
    __qt_internal_show_error_no_emscripten_toolchain_file_found_when_using_qt()
endif()
")
    endif()

    string(REPLACE ";" "\n" init_additional_used_variables
        "${init_additional_used_variables}")
    string(REPLACE ";" "\n" init_vcpkg "${init_vcpkg}")

    string(REPLACE ";" "\n" init_platform "${init_platform}")
    string(REPLACE "LITERAL_SEMICOLON" ";" init_platform "${init_platform}")

    string(REPLACE ";" "\n" init_post_chainload_toolchain "${init_post_chainload_toolchain}")
    string(REPLACE "LITERAL_SEMICOLON" ";" init_post_chainload_toolchain
           "${init_post_chainload_toolchain}")

    qt_compute_relative_path_from_cmake_config_dir_to_prefix()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt.toolchain.cmake.in"
        "${__GlobalConfig_build_dir}/qt.toolchain.cmake" @ONLY)
    qt_install(FILES "${__GlobalConfig_build_dir}/qt.toolchain.cmake"
               DESTINATION "${__GlobalConfig_install_dir}" COMPONENT Devel)
endfunction()
