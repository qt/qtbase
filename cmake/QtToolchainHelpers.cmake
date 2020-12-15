# Create a CMake toolchain file for convenient configuration of both internal Qt builds
# as well as CMake application projects.
# Expects various global variables to be set.
function(qt_internal_create_toolchain_file)
    if(NOT "${QT_HOST_PATH}" STREQUAL "")
        # TODO: Figure out how to make these relocatable.

        get_filename_component(__qt_host_path_absolute "${QT_HOST_PATH}" ABSOLUTE)
        set(init_qt_host_path "
    set(__qt_initial_qt_host_path \"${__qt_host_path_absolute}\")
    if(NOT DEFINED QT_HOST_PATH AND EXISTS \"\${__qt_initial_qt_host_path}\")
        set(QT_HOST_PATH \"\${__qt_initial_qt_host_path}\" CACHE PATH \"\" FORCE)
    endif()")

        get_filename_component(__qt_host_path_cmake_dir_absolute
            "${Qt${PROJECT_VERSION_MAJOR}HostInfo_DIR}/.." ABSOLUTE)
        set(init_qt_host_path_cmake_dir
            "
    set(__qt_initial_qt_host_path_cmake_dir \"${__qt_host_path_cmake_dir_absolute}\")
    if(NOT DEFINED QT_HOST_PATH_CMAKE_DIR AND EXISTS \"\${__qt_initial_qt_host_path_cmake_dir}\")
        set(QT_HOST_PATH_CMAKE_DIR \"\${__qt_initial_qt_host_path_cmake_dir}\" CACHE PATH \"\" FORCE)
    endif()")

        set(init_qt_host_path_checks "
    if(\"\${QT_HOST_PATH}\" STREQUAL \"\" OR NOT EXISTS \"\${QT_HOST_PATH}\")
        message(FATAL_ERROR \"To use a cross-compiled Qt, please specify a path to a host Qt installation by setting the QT_HOST_PATH cache variable.\")
    endif()
    if(\"\${QT_HOST_PATH_CMAKE_DIR}\" STREQUAL \"\" OR NOT EXISTS \"\${QT_HOST_PATH_CMAKE_DIR}\")
        message(FATAL_ERROR \"To use a cross-compiled Qt, please specify a path to a host Qt installation CMake directory by setting the QT_HOST_PATH_CMAKE_DIR cache variable.\")
    endif()")
    endif()

    if(CMAKE_TOOLCHAIN_FILE)
        file(TO_CMAKE_PATH "${CMAKE_TOOLCHAIN_FILE}" __qt_chainload_toolchain_file)
        set(init_original_toolchain_file
            "set(__qt_chainload_toolchain_file \"${__qt_chainload_toolchain_file}\")")
    endif()

    if(VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        list(APPEND init_vcpkg
             "set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE \"${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}\")")
    endif()

    if(VCPKG_TARGET_TRIPLET)
        list(APPEND init_vcpkg
             "set(VCPKG_TARGET_TRIPLET \"${VCPKG_TARGET_TRIPLET}\" CACHE STRING \"\")")
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
        # For simulator_and_device build, we should not explicitly set the sysroot.
        list(LENGTH CMAKE_OSX_ARCHITECTURES _qt_osx_architectures_count)
        if(CMAKE_OSX_SYSROOT AND NOT _qt_osx_architectures_count GREATER 1 AND UIKIT)
            list(APPEND init_platform "
    set(__qt_initial_cmake_osx_sysroot \"${CMAKE_OSX_SYSROOT}\")
    if(NOT DEFINED CMAKE_OSX_SYSROOT AND EXISTS \"\${__qt_initial_cmake_osx_sysroot}\")
        set(CMAKE_OSX_SYSROOT \"\${__qt_initial_cmake_osx_sysroot}\" CACHE PATH \"\")
    endif()")
        endif()

        if(CMAKE_OSX_DEPLOYMENT_TARGET)
            list(APPEND init_platform
                "set(CMAKE_OSX_DEPLOYMENT_TARGET \"${CMAKE_OSX_DEPLOYMENT_TARGET}\" CACHE STRING \"\")")
        endif()

        if(UIKIT)
            list(APPEND init_platform
                "set(CMAKE_SYSTEM_NAME \"${CMAKE_SYSTEM_NAME}\" CACHE STRING \"\")")
            set(_qt_osx_architectures_escaped "${CMAKE_OSX_ARCHITECTURES}")
            string(REPLACE ";" "LITERAL_SEMICOLON"
                _qt_osx_architectures_escaped "${_qt_osx_architectures_escaped}")
            list(APPEND init_platform
                "set(CMAKE_OSX_ARCHITECTURES \"${_qt_osx_architectures_escaped}\" CACHE STRING \"\")")

            list(APPEND init_platform "if(CMAKE_GENERATOR STREQUAL \"Xcode\" AND NOT QT_NO_XCODE_EMIT_EPN)")
            list(APPEND init_platform "    set_property(GLOBAL PROPERTY XCODE_EMIT_EFFECTIVE_PLATFORM_NAME OFF)")
            list(APPEND init_platform "endif()")
        endif()
    elseif(ANDROID)
        foreach(var ANDROID_NATIVE_API_LEVEL ANDROID_STL ANDROID_ABI
                ANDROID_SDK_ROOT ANDROID_NDK_ROOT)
            list(APPEND init_additional_used_variables
                "list(APPEND __qt_toolchain_used_variables ${var})")
        endforeach()
        list(APPEND init_platform
             "set(ANDROID_NATIVE_API_LEVEL \"${ANDROID_NATIVE_API_LEVEL}\" CACHE STRING \"\")")
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
    endif()

    string(REPLACE ";" "\n" init_additional_used_variables
        "${init_additional_used_variables}")
    string(REPLACE ";" "\n" init_vcpkg "${init_vcpkg}")
    string(REPLACE ";" "\n" init_platform "${init_platform}")
    string(REPLACE "LITERAL_SEMICOLON" ";" init_platform "${init_platform}")
    qt_compute_relative_path_from_cmake_config_dir_to_prefix()
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/qt.toolchain.cmake.in"
        "${__GlobalConfig_build_dir}/qt.toolchain.cmake" @ONLY)
    qt_install(FILES "${__GlobalConfig_build_dir}/qt.toolchain.cmake"
               DESTINATION "${__GlobalConfig_install_dir}" COMPONENT Devel)
endfunction()
