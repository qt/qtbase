
function(qt_internal_set_warnings_are_errors_flags target)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # Regular clang 3.0+
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "3.0.0")
            target_compile_options("${target}" INTERFACE -Werror -Wno-error=\#warnings -Wno-error=deprecated-declarations)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        # using AppleClang
        # Apple clang 4.0+
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "4.0.0")
            target_compile_options("${target}" INTERFACE -Werror -Wno-error=\#warnings -Wno-error=deprecated-declarations)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # using GCC
        target_compile_options("${target}" INTERFACE -Werror -Wno-error=cpp -Wno-error=deprecated-declarations)

        # GCC prints this bogus warning, after it has inlined a lot of code
        # error: assuming signed overflow does not occur when assuming that (X + c) < X is always false
        target_compile_options("${target}" INTERFACE -Wno-error=strict-overflow)

        # GCC 7 includes -Wimplicit-fallthrough in -Wextra, but Qt is not yet free of implicit fallthroughs.
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "7.0.0")
            target_compile_options("${target}" INTERFACE -Wno-error=implicit-fallthrough)
        endif()

        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9.0.0")
            # GCC 9 introduced these but we are not clean for it.
            target_compile_options("${target}" INTERFACE -Wno-error=deprecated-copy -Wno-error=redundant-move -Wno-error=init-list-lifetime)
        endif()

        # Work-around for bug https://code.google.com/p/android/issues/detail?id=58135
        if (ANDROID)
            target_compile_options("${target}" INTERFACE -Wno-error=literal-suffix)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
        # Intel CC 13.0 +, on Linux only
        if (LINUX)
            if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "13.0.0")
                # 177: function "entity" was declared but never referenced
                #      (too aggressive; ICC reports even for functions created due to template instantiation)
                # 1224: #warning directive
                # 1478: function "entity" (declared at line N) was declared deprecated
                # 1786: function "entity" (declared at line N of "file") was declared deprecated ("message")
                # 1881: argument must be a constant null pointer value
                #      (NULL in C++ is usually a literal 0)
                target_compile_options("${target}" INTERFACE -Werror -ww177,1224,1478,1786,1881)
            endif()
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # using Visual Studio C++
        target_compile_options("${target}" INTERFACE /WX)
    endif()
endfunction()

add_library(PlatformModuleInternal INTERFACE)
add_library(Qt::PlatformModuleInternal ALIAS PlatformModuleInternal)

add_library(PlatformPluginInternal INTERFACE)
add_library(Qt::PlatformPluginInternal ALIAS PlatformPluginInternal)

add_library(PlatformToolInternal INTERFACE)
add_library(Qt::PlatformToolInternal ALIAS PlatformToolInternal)

if(WARNINGS_ARE_ERRORS)
    qt_internal_set_warnings_are_errors_flags(PlatformModuleInternal)
    qt_internal_set_warnings_are_errors_flags(PlatformPluginInternal)
    qt_internal_set_warnings_are_errors_flags(PlatformToolInternal)
endif()
if(WIN32)
    # Needed for M_PI define. Same as mkspecs/features/qt_module.prf.
    # It's set for every module being built, but it's not propagated to user apps.
    target_compile_definitions(PlatformModuleInternal INTERFACE _USE_MATH_DEFINES)
endif()
if(FEATURE_largefile AND UNIX)
    target_compile_definitions(PlatformModuleInternal
                               INTERFACE "_LARGEFILE64_SOURCE;_LARGEFILE_SOURCE")
endif()

# We can't use the gold linker on android with the NDK, which is the default
# linker. To build our own target we will use the lld linker.
if (ANDROID)
    target_link_options(PlatformModuleInternal INTERFACE -fuse-ld=lld)
endif()
