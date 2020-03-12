
function(qt_internal_set_warnings_are_errors_flags target)
    set(flags "")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # Regular clang 3.0+
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "3.0.0")
            list(APPEND flags -Werror -Wno-error=\#warnings -Wno-error=deprecated-declarations)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        # using AppleClang
        # Apple clang 4.0+
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "4.0.0" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS_EQUAL "9.2")
            list(APPEND flags -Werror -Wno-error=\#warnings -Wno-error=deprecated-declarations)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        # using GCC
        list(APPEND flags -Werror -Wno-error=cpp -Wno-error=deprecated-declarations)

        # GCC prints this bogus warning, after it has inlined a lot of code
        # error: assuming signed overflow does not occur when assuming that (X + c) < X is always false
        list(APPEND flags -Wno-error=strict-overflow)

        # GCC 7 includes -Wimplicit-fallthrough in -Wextra, but Qt is not yet free of implicit fallthroughs.
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "7.0.0")
            list(APPEND flags -Wno-error=implicit-fallthrough)
        endif()

        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9.0.0")
            # GCC 9 introduced these but we are not clean for it.
            list(APPEND flags -Wno-error=deprecated-copy -Wno-error=redundant-move -Wno-error=init-list-lifetime)
        endif()

        # Work-around for bug https://code.google.com/p/android/issues/detail?id=58135
        if (ANDROID)
            list(APPEND flags -Wno-error=literal-suffix)
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
                list(APPEND flags -Werror -ww177,1224,1478,1786,1881)
            endif()
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        # In qmake land, currently warnings as errors are only enabled for
        # MSVC 2012, 2013, 2015.
        # Respectively MSVC_VERRSIONs are: 1700-1799, 1800-1899, 1900-1909.
        if(MSVC_VERSION GREATER_EQUAL 1700 AND MSVC_VERSION LESS_EQUAL 1909)
            list(APPEND flags /WX)
        endif()
    endif()
    set(add_flags "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_SKIP_WARNINGS_ARE_ERRORS>>>")
    set(flags_generator_expression "$<${add_flags}:${flags}>")
    target_compile_options("${target}" INTERFACE "${flags_generator_expression}")
endfunction()

add_library(PlatformCommonInternal INTERFACE)
add_library(Qt::PlatformCommonInternal ALIAS PlatformCommonInternal)

add_library(PlatformModuleInternal INTERFACE)
add_library(Qt::PlatformModuleInternal ALIAS PlatformModuleInternal)
target_link_libraries(PlatformModuleInternal INTERFACE PlatformCommonInternal)

add_library(PlatformPluginInternal INTERFACE)
add_library(Qt::PlatformPluginInternal ALIAS PlatformPluginInternal)
target_link_libraries(PlatformPluginInternal INTERFACE PlatformCommonInternal)

add_library(PlatformToolInternal INTERFACE)
add_library(Qt::PlatformToolInternal ALIAS PlatformToolInternal)
target_link_libraries(PlatformToolInternal INTERFACE PlatformCommonInternal)

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

target_compile_definitions(PlatformCommonInternal INTERFACE $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>)

if(APPLE_OSX)
    target_compile_definitions(PlatformCommonInternal INTERFACE GL_SILENCE_DEPRECATION)
elseif(APPLE_UIKIT)
    target_compile_definitions(PlatformCommonInternal INTERFACE GLES_SILENCE_DEPRECATION)
endif()

if(APPLE_UIKIT)
    # Do what mkspecs/features/uikit/default_pre.prf does, aka enable sse2 for
    # simulator_and_device_builds.
    if(FEATURE_simulator_and_device)
        # Setting the definition on PlatformCommonInternal behaves slightly differently from what
        # is done in qmake land. This way the define is not propagated to tests, examples, or
        # user projects built with qmake, but only modules, plugins and tools.
        # TODO: Figure out if this ok or not (sounds ok to me).
        target_compile_definitions(PlatformCommonInternal INTERFACE QT_COMPILER_SUPPORTS_SSE2)
    endif()
endif()

# Taken from mkspecs/common/msvc-version.conf and mkspecs/common/msvc-desktop.conf
if (MSVC)
    if (MSVC_VERSION GREATER_EQUAL 1799)
        target_compile_options(PlatformCommonInternal INTERFACE
            -FS
            -Zc:rvalueCast
            -Zc:inline
    )
    endif()
    if (MSVC_VERSION GREATER_EQUAL 1899)
        target_compile_options(PlatformCommonInternal INTERFACE
            -Zc:strictStrings
            -Zc:throwingNew
        )
    endif()
    if (MSVC_VERSION GREATER_EQUAL 1909)
        target_compile_options(PlatformCommonInternal INTERFACE
            -Zc:referenceBinding
        )
    endif()

    target_compile_options(PlatformCommonInternal INTERFACE -Zc:wchar_t -utf-8)

    target_link_options(PlatformCommonInternal INTERFACE
        -DYNAMICBASE -NXCOMPAT
        $<$<CONFIG:Release>:-OPT:REF>
        $<$<CONFIG:RelWithDebInfo>:-OPT:REF>
    )
endif()
