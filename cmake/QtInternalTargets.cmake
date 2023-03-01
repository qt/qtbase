
function(qt_internal_set_warnings_are_errors_flags target)
    set(flags "")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND NOT MSVC)
        # Regular clang 3.0+
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "3.0.0")
            list(APPEND flags -Werror -Wno-error=\#warnings -Wno-error=deprecated-declarations)
        endif()
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0.0")
            # We do mixed enum arithmetic all over the place:
            list(APPEND flags -Wno-error=deprecated-enum-enum-conversion)
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
            # GCC 9 introduced -Wformat-overflow in -Wall, but it is buggy:
            list(APPEND flags -Wno-error=format-overflow)
        endif()

        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0.0")
            # GCC 10 has a number of bugs in -Wstringop-overflow. Do not make them an error.
            # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92955
            # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94335
            # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=101134
            list(APPEND flags -Wno-error=stringop-overflow)
        endif()

        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "11.0.0")
            # We do mixed enum arithmetic all over the place:
            list(APPEND flags -Wno-error=deprecated-enum-enum-conversion -Wno-error=deprecated-enum-float-conversion)
        endif()

        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "11.0.0" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.2.0")
            # GCC 11.1 has a regression in the integrated preprocessor, so disable it as a workaround (QTBUG-93360)
            # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100796
            # This in turn triggers a fallthrough warning in cborparser.c, so we disable this warning.
            list(APPEND flags -no-integrated-cpp -Wno-implicit-fallthrough)
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
        # Only enable for versions of MSVC that are known to work
        # 1929 is Visual Studio 2019 version 16.0
        if(MSVC_VERSION LESS_EQUAL 1929)
            list(APPEND flags /WX)
        endif()
    endif()
    set(warnings_are_errors_enabled_genex
        "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_SKIP_WARNINGS_ARE_ERRORS>>>")

    # Apprently qmake only adds -Werror to CXX and OBJCXX files, not C files. We have to do the
    # same otherwise MinGW builds break when building 3rdparty\md4c\md4c.c (and probably on other
    # platforms too).
    set(cxx_only_genex "$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>")
    set(final_condition_genex "$<AND:${warnings_are_errors_enabled_genex},${cxx_only_genex}>")
    set(flags_generator_expression "$<${final_condition_genex}:${flags}>")
    target_compile_options("${target}" INTERFACE "${flags_generator_expression}")
endfunction()

# The function adds a global 'definition' to the platform internal targets and the target
# property-based switch to disable the definition.
# Arguments:
#     VALUE optional value that the definition will take.
#     SCOPE the list of scopes the definition needs to be set for. If the SCOPE is not specified the
#        definition is added to PlatformCommonInternal target.
#        Possible values:
#            MODULE - set the definition for all Qt modules
#            PLUGIN - set the definition for all Qt plugins
#            TOOL - set the definition for all Qt tools
#            APP - set the definition for all Qt applications
#        TODO: Add a tests specific platform target and the definition scope for it.
function(qt_internal_add_global_definition definition)
    set(optional_args)
    set(single_value_args VALUE)
    set(multi_value_args SCOPE)
    cmake_parse_arguments(args
        "${optional_args}"
        "${single_value_args}"
        "${multi_value_args}"
        ${ARGN}
    )

    set(scope_MODULE PlatformModuleInternal)
    set(scope_PLUGIN PlatformPluginInternal)
    set(scope_TOOL PlatformToolInternal)
    set(scope_APP PlatformAppInternal)

    set(undef_property_name "QT_INTERNAL_UNDEF_${definition}")

    if(DEFINED arg_VALUE)
        set(definition "${definition}=${arg_VALUE}")
    endif()

    set(definition_genex
        "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:${undef_property_name}>>>:${definition}>")

    if(NOT DEFINED arg_SCOPE)
        target_compile_definitions(PlatformCommonInternal INTERFACE "${definition_genex}")
    else()
        foreach(scope IN LISTS arg_SCOPE)
            if(NOT DEFINED scope_${scope})
                message(FATAL_ERROR "Unknown scope ${scope}.")
            endif()
            target_compile_definitions("${scope_${scope}}" INTERFACE "${definition_genex}")
        endforeach()
    endif()
endfunction()

add_library(PlatformCommonInternal INTERFACE)
add_library(Qt::PlatformCommonInternal ALIAS PlatformCommonInternal)
target_link_libraries(PlatformCommonInternal INTERFACE Platform)

add_library(PlatformModuleInternal INTERFACE)
add_library(Qt::PlatformModuleInternal ALIAS PlatformModuleInternal)
target_link_libraries(PlatformModuleInternal INTERFACE PlatformCommonInternal)

add_library(PlatformPluginInternal INTERFACE)
add_library(Qt::PlatformPluginInternal ALIAS PlatformPluginInternal)
target_link_libraries(PlatformPluginInternal INTERFACE PlatformCommonInternal)

add_library(PlatformAppInternal INTERFACE)
add_library(Qt::PlatformAppInternal ALIAS PlatformAppInternal)
target_link_libraries(PlatformAppInternal INTERFACE PlatformCommonInternal)

add_library(PlatformToolInternal INTERFACE)
add_library(Qt::PlatformToolInternal ALIAS PlatformToolInternal)
target_link_libraries(PlatformToolInternal INTERFACE PlatformAppInternal)

qt_internal_add_global_definition(QT_NO_JAVA_STYLE_ITERATORS)
qt_internal_add_global_definition(QT_NO_NARROWING_CONVERSIONS_IN_CONNECT)

if(WARNINGS_ARE_ERRORS)
    qt_internal_set_warnings_are_errors_flags(PlatformModuleInternal)
    qt_internal_set_warnings_are_errors_flags(PlatformPluginInternal)
    qt_internal_set_warnings_are_errors_flags(PlatformAppInternal)
endif()
if(WIN32)
    # Needed for M_PI define. Same as mkspecs/features/qt_module.prf.
    # It's set for every module being built, but it's not propagated to user apps.
    target_compile_definitions(PlatformModuleInternal INTERFACE _USE_MATH_DEFINES)
endif()
if(FEATURE_largefile AND UNIX)
    target_compile_definitions(PlatformCommonInternal
                               INTERFACE "_LARGEFILE64_SOURCE;_LARGEFILE_SOURCE")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Clang will otherwise show error about inline method conflicting with dllimport class attribute in tools
    # (this was tested with Clang 10)
    #    error: 'QString::operator[]' redeclared inline; 'dllimport' attribute ignored [-Werror,-Wignored-attributes]
    target_compile_options(PlatformCommonInternal INTERFACE -Wno-ignored-attributes)
endif()

# We can't use the gold linker on android with the NDK, which is the default
# linker. To build our own target we will use the lld linker.
# TODO: Why not?
# Linking Android libs with lld on Windows sometimes deadlocks. Don't use lld on
# Windows. qmake doesn't use lld to build Android on any host platform.
if (ANDROID AND NOT CMAKE_HOST_WIN32)
    target_link_options(PlatformModuleInternal INTERFACE -fuse-ld=lld)
endif()

target_compile_definitions(PlatformCommonInternal INTERFACE QT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
target_compile_definitions(PlatformCommonInternal INTERFACE $<$<NOT:$<CONFIG:Debug>>:QT_NO_DEBUG>)

function(qt_internal_apply_bitcode_flags target)
    # See mkspecs/features/uikit/bitcode.prf
    set(release_flags "-fembed-bitcode")
    set(debug_flags "-fembed-bitcode-marker")

    set(is_release_genex "$<NOT:$<CONFIG:Debug>>")
    set(flags_genex "$<IF:${is_release_genex},${release_flags},${debug_flags}>")
    set(is_enabled_genex "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_NO_BITCODE>>>")

    set(bitcode_flags "$<${is_enabled_genex}:${flags_genex}>")

    target_compile_options("${target}" INTERFACE ${bitcode_flags})
endfunction()

# Apple deprecated the entire OpenGL API in favor of Metal, which
# we are aware of, so silence the deprecation warnings in code.
# This does not apply to user-code, which will need to silence
# their own warnings if they use the deprecated APIs explicitly.
if(MACOS)
    target_compile_definitions(PlatformCommonInternal INTERFACE GL_SILENCE_DEPRECATION)
elseif(UIKIT)
    target_compile_definitions(PlatformCommonInternal INTERFACE GLES_SILENCE_DEPRECATION)
endif()

if(MSVC)
    target_compile_definitions(PlatformCommonInternal INTERFACE
        "_CRT_SECURE_NO_WARNINGS"
        "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:_WINDLL>"
    )
endif()

if(UIKIT)
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

if(WASM AND QT_FEATURE_sse2)
    target_compile_definitions(PlatformCommonInternal INTERFACE QT_COMPILER_SUPPORTS_SSE2)
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
        )
        if (NOT CLANG)
            target_compile_options(PlatformCommonInternal INTERFACE
                -Zc:throwingNew
            )
        endif()
    endif()
    if (MSVC_VERSION GREATER_EQUAL 1909 AND NOT CLANG)
        target_compile_options(PlatformCommonInternal INTERFACE
            -Zc:referenceBinding
        )
    endif()
    if (MSVC_VERSION GREATER_EQUAL 1919 AND NOT CLANG)
        target_compile_options(PlatformCommonInternal INTERFACE
            -Zc:externConstexpr
        )
    endif()

    target_compile_options(PlatformCommonInternal INTERFACE
        -Zc:wchar_t
        -bigobj
    )

    target_compile_options(PlatformCommonInternal INTERFACE
        $<$<NOT:$<CONFIG:Debug>>:-guard:cf>
    )

    target_link_options(PlatformCommonInternal INTERFACE
        -DYNAMICBASE -NXCOMPAT
        $<$<NOT:$<CONFIG:Debug>>:-OPT:REF -OPT:ICF -GUARD:CF>
    )
endif()

if(MINGW)
    target_compile_options(PlatformCommonInternal INTERFACE -Wa,-mbig-obj)
endif()

if (GCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "9.2")
    target_compile_options(PlatformCommonInternal INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-Wsuggest-override>)
endif()

if(QT_FEATURE_force_asserts)
    target_compile_definitions(PlatformCommonInternal INTERFACE QT_FORCE_ASSERTS)
endif()

if(DEFINED QT_EXTRA_DEFINES)
    target_compile_definitions(PlatformCommonInternal INTERFACE ${QT_EXTRA_DEFINES})
endif()

if(DEFINED QT_EXTRA_INCLUDEPATHS)
    target_include_directories(PlatformCommonInternal INTERFACE ${QT_EXTRA_INCLUDEPATHS})
endif()

if(DEFINED QT_EXTRA_LIBDIRS)
    target_link_directories(PlatformCommonInternal INTERFACE ${QT_EXTRA_LIBDIRS})
endif()

if(DEFINED QT_EXTRA_FRAMEWORKPATHS AND APPLE)
    list(TRANSFORM QT_EXTRA_FRAMEWORKPATHS PREPEND "-F" OUTPUT_VARIABLE __qt_fw_flags)
    target_compile_options(PlatformCommonInternal INTERFACE ${__qt_fw_flags})
    target_link_options(PlatformCommonInternal INTERFACE ${__qt_fw_flags})
    unset(__qt_fw_flags)
endif()

qt_internal_get_active_linker_flags(__qt_internal_active_linker_flags)
if(__qt_internal_active_linker_flags)
    target_link_options(PlatformCommonInternal INTERFACE "${__qt_internal_active_linker_flags}")
endif()
unset(__qt_internal_active_linker_flags)

if(QT_FEATURE_enable_gdb_index)
    target_link_options(PlatformCommonInternal INTERFACE "-Wl,--gdb-index")
endif()

if(QT_FEATURE_enable_new_dtags)
    target_link_options(PlatformCommonInternal INTERFACE "-Wl,--enable-new-dtags")
endif()

function(qt_get_implicit_sse2_genex_condition out_var)
    set(is_shared_lib "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>")
    set(is_static_lib "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,STATIC_LIBRARY>")
    set(is_static_qt_build "$<NOT:$<BOOL:${QT_BUILD_SHARED_LIBS}>>")
    set(is_staitc_lib_during_static_qt_build "$<AND:${is_static_qt_build},${is_static_lib}>")
    set(enable_sse2_condition "$<OR:${is_shared_lib},${is_staitc_lib_during_static_qt_build}>")
    set(${out_var} "${enable_sse2_condition}" PARENT_SCOPE)
endfunction()

function(qt_auto_detect_implicit_sse2)
    # sse2 configuration adjustment in qt_module.prf
    # If the compiler supports SSE2, enable it unconditionally in all of Qt shared libraries
    # (and only the libraries). This is not expected to be a problem because:
    # - on Windows, sharing of libraries is uncommon
    # - on Mac OS X, all x86 CPUs already have SSE2 support (we won't even reach here)
    # - on Linux, the dynamic loader can find the libraries on LIBDIR/sse2/
    # The last guarantee does not apply to executables and plugins, so we can't enable for them.
    set(__implicit_sse2_for_qt_modules_enabled FALSE PARENT_SCOPE)
    if(TEST_subarch_sse2 AND NOT TEST_arch_${TEST_architecture_arch}_subarch_sse2)
        qt_get_implicit_sse2_genex_condition(enable_sse2_condition)
        set(enable_sse2_genex "$<${enable_sse2_condition}:${QT_CFLAGS_SSE2}>")
        target_compile_options(PlatformModuleInternal INTERFACE ${enable_sse2_genex})
        set(__implicit_sse2_for_qt_modules_enabled TRUE PARENT_SCOPE)
    endif()
endfunction()
qt_auto_detect_implicit_sse2()

function(qt_auto_detect_fpmath)
    # fpmath configuration adjustment in qt_module.prf
    set(fpmath_supported FALSE)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "3.4")
            set(fpmath_supported TRUE)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "5.1")
            set(fpmath_supported TRUE)
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set(fpmath_supported TRUE)
    endif()
    if(fpmath_supported AND TEST_architecture_arch STREQUAL "i386" AND __implicit_sse2_for_qt_modules_enabled)
        qt_get_implicit_sse2_genex_condition(enable_sse2_condition)
        set(enable_fpmath_genex "$<${enable_sse2_condition}:-mfpmath=sse>")
        target_compile_options(PlatformModuleInternal INTERFACE ${enable_fpmath_genex})
    endif()
endfunction()
qt_auto_detect_fpmath()

function(qt_handle_apple_app_extension_api_only)
    if(APPLE)
        # Build Qt libraries with -fapplication-extension. Needed to avoid linker warnings
        # transformed into errors on darwin platforms.
        set(flags "-fapplication-extension")

        # The flags should only be applied to internal Qt libraries like modules and plugins.
        # The reason why we use a custom property to apply the flags is because there's no other
        # way to prevent the link options spilling out into user projects if the target that links
        # against PlatformXInternal is a static library.
        # The exported static library's INTERFACE_LINK_LIBRARIES property would contain
        # $<LINK_ONLY:PlatformXInternal> and PlatformXInternal's INTERFACE_LINK_OPTIONS would be
        # applied to a user project target.
        # So to contain the spilling out of the flags, we ensure the link options are only added
        # to internal Qt libraries that are marked with the property.
        set(not_disabled "$<NOT:$<BOOL:$<TARGET_PROPERTY:QT_NO_APP_EXTENSION_ONLY_API>>>")
        set(is_qt_internal_library "$<BOOL:$<TARGET_PROPERTY:_qt_is_internal_library>>")

        set(condition "$<AND:${not_disabled},${is_qt_internal_library}>")

        set(flags "$<${condition}:${flags}>")
        target_compile_options(PlatformModuleInternal INTERFACE ${flags})
        target_link_options(PlatformModuleInternal INTERFACE ${flags})
        target_compile_options(PlatformPluginInternal INTERFACE ${flags})
        target_link_options(PlatformPluginInternal INTERFACE ${flags})
    endif()
endfunction()

qt_handle_apple_app_extension_api_only()
