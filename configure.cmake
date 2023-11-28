# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#### Inputs



#### Libraries

qt_find_package(WrapSystemZLIB 1.0.8 PROVIDED_TARGETS WrapSystemZLIB::WrapSystemZLIB MODULE_NAME global QMAKE_LIB zlib)
# Work around global target promotion failure when WrapZLIB is used on APPLE platforms.
# What ends up happening is that the ZLIB::ZLIB target is not promoted to global by qt_find_package,
# then qt_find_package(WrapSystemPNG) tries to find its dependency ZLIB::ZLIB, sees it's not global
# and tries to promote it to global, but fails because the directory scope of the PNG package is
# different (src/gui) from where ZLIB was originally found (qtbase root).
# To avoid that, just manually promote the target to global here.
if(TARGET ZLIB::ZLIB)
    set_property(TARGET ZLIB::ZLIB PROPERTY IMPORTED_GLOBAL TRUE)
endif()

qt_find_package(WrapOpenSSLHeaders PROVIDED_TARGETS WrapOpenSSLHeaders::WrapOpenSSLHeaders MODULE_NAME core)
# openssl_headers
# OPENSSL_VERSION_MAJOR is not defined for OpenSSL 1.1.1
qt_config_compile_test(opensslv11_headers
    LABEL "opensslv11_headers"
    LIBRARIES
        WrapOpenSSLHeaders::WrapOpenSSLHeaders
    CODE
"#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if !defined(OPENSSL_VERSION_NUMBER) || defined(OPENSSL_VERSION_MAJOR) || OPENSSL_VERSION_NUMBER-0 < 0x10101000L
#  error OpenSSL >= 1.1.1 is required
#endif
#if !defined(OPENSSL_NO_EC) && !defined(SSL_CTRL_SET_CURVES)
#  error OpenSSL was reported as >= 1.1.1 but is missing required features, possibly it is libressl which is unsupported
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
")

qt_find_package(WrapOpenSSL PROVIDED_TARGETS WrapOpenSSL::WrapOpenSSL MODULE_NAME core QMAKE_LIB openssl)
# openssl
# OPENSSL_VERSION_MAJOR is not defined for OpenSSL 1.1.1
qt_config_compile_test(opensslv11
    LABEL "opensslv11"
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
    CODE
"#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if !defined(OPENSSL_VERSION_NUMBER) || defined(OPENSSL_VERSION_MAJOR) || OPENSSL_VERSION_NUMBER-0 < 0x10101000L
#  error OpenSSL >= 1.1.1 is required
#endif
#if !defined(OPENSSL_NO_EC) && !defined(SSL_CTRL_SET_CURVES)
#  error OpenSSL was reported as >= 1.1.1 but is missing required features, possibly it is libressl which is unsupported
#endif

int main(void)
{
    /* BEGIN TEST: */
SSL_free(SSL_new(0));
    /* END TEST: */
    return 0;
}
")

# opensslv30
# openssl_headers
qt_config_compile_test(opensslv30_headers
    LABEL "opensslv30_headers"
    LIBRARIES
        WrapOpenSSLHeaders::WrapOpenSSLHeaders
    CODE
"#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if !OPENSSL_VERSION_PREREQ(3,0)
#  error OpenSSL >= 3.0 is required
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
")
qt_config_compile_test(opensslv30
    LABEL "opensslv30"
    LIBRARIES
        WrapOpenSSL::WrapOpenSSL
    CODE
"#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if !OPENSSL_VERSION_PREREQ(3,0)
#  error OpenSSL >= 3.0 is required
#endif

int main(void)
{
    /* BEGIN TEST: */
SSL_free(SSL_new(0));
    /* END TEST: */
    return 0;
}
")

qt_find_package(WrapZSTD 1.3 PROVIDED_TARGETS WrapZSTD::WrapZSTD MODULE_NAME global QMAKE_LIB zstd)
qt_find_package(WrapDBus1 1.2 PROVIDED_TARGETS dbus-1 MODULE_NAME global QMAKE_LIB dbus)
qt_find_package(Libudev PROVIDED_TARGETS PkgConfig::Libudev MODULE_NAME global QMAKE_LIB libudev)
qt_find_package(LTTngUST PROVIDED_TARGETS LTTng::UST MODULE_NAME core QMAKE_LIB lttng-ust)
qt_add_qmake_lib_dependency(lttng-ust libdl)


#### Early-evaluated, Linker-related Tests and Features

qt_internal_check_if_linker_is_available(use_bfd_linker
    LABEL "bfd linker"
    FLAG "-fuse-ld=bfd"
    )

qt_internal_check_if_linker_is_available(use_gold_linker
    LABEL "gold linker"
    FLAG "-fuse-ld=gold"
    )

qt_internal_check_if_linker_is_available(use_lld_linker
    LABEL "lld linker"
    FLAG "-fuse-ld=lld"
    )

# We set an invalid flag as a default flag so the compile test fails
# in case if no mold is found in PATH.
set(__qt_internal_mold_linker_flags "-Wl,-invalid-flag")
if(NOT QT_CONFIGURE_RUNNING)
    qt_internal_get_mold_linker_flags(__qt_internal_mold_linker_flags)
endif()
qt_internal_check_if_linker_is_available(use_mold_linker
    LABEL "mold linker"
    FLAG "${__qt_internal_mold_linker_flags}"
    )
unset(__qt_internal_mold_linker_flags)

qt_feature("use_bfd_linker"
    PRIVATE
    LABEL "bfd"
    AUTODETECT false
    CONDITION NOT MSVC AND NOT INTEGRITY AND NOT WASM AND TEST_use_bfd_linker
    ENABLE INPUT_linker STREQUAL 'bfd'
    DISABLE INPUT_linker STREQUAL 'gold' OR INPUT_linker STREQUAL 'lld'
            OR INPUT_linker STREQUAL 'mold'
)
qt_feature_config("use_bfd_linker" QMAKE_PRIVATE_CONFIG)

qt_feature("use_gold_linker_alias"
    AUTODETECT false
    CONDITION NOT WIN32 AND NOT INTEGRITY AND NOT WASM AND TEST_use_gold_linker
)
qt_feature("use_gold_linker"
    PRIVATE
    LABEL "gold"
    AUTODETECT false
    CONDITION NOT WIN32 AND NOT INTEGRITY AND NOT WASM AND NOT rtems AND TEST_use_gold_linker
    ENABLE INPUT_linker STREQUAL 'gold' OR QT_FEATURE_use_gold_linker_alias
    DISABLE INPUT_linker STREQUAL 'bfd' OR INPUT_linker STREQUAL 'lld'
            OR INPUT_linker STREQUAL 'mold'
)
qt_feature_config("use_gold_linker" QMAKE_PRIVATE_CONFIG)

qt_feature("use_lld_linker"
    PRIVATE
    LABEL "lld"
    AUTODETECT false
    CONDITION NOT MSVC AND NOT INTEGRITY AND NOT WASM AND TEST_use_lld_linker
    ENABLE INPUT_linker STREQUAL 'lld'
    DISABLE INPUT_linker STREQUAL 'bfd' OR INPUT_linker STREQUAL 'gold'
            OR INPUT_linker STREQUAL 'mold'
)
qt_feature_config("use_lld_linker" QMAKE_PRIVATE_CONFIG)

qt_feature("use_mold_linker"
    PRIVATE
    LABEL "mold"
    AUTODETECT FALSE
    CONDITION NOT WIN32 AND NOT INTEGRITY AND NOT WASM AND TEST_use_mold_linker
    ENABLE INPUT_linker STREQUAL 'mold'
    DISABLE INPUT_linker STREQUAL 'bfd' OR INPUT_linker STREQUAL 'gold'
            OR INPUT_linker STREQUAL 'lld'
)
qt_feature_config("use_mold_linker" QMAKE_PRIVATE_CONFIG)

if(NOT QT_CONFIGURE_RUNNING)
    qt_evaluate_feature(use_bfd_linker)
    qt_evaluate_feature(use_gold_linker_alias)
    qt_evaluate_feature(use_gold_linker)
    qt_evaluate_feature(use_lld_linker)
    qt_evaluate_feature(use_mold_linker)
endif()


#### Tests

# machineTuple
qt_config_compile_test_machine_tuple("machine tuple")

# cxx20
qt_config_compile_test(cxx20
    LABEL "C++20 support"
    CODE
"#if __cplusplus > 201703L
// Compiler claims to support C++20, trust it
#else
#  error __cplusplus must be > 201703L (the value for C++17)
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
"
    CXX_STANDARD 20
)

qt_config_compile_test(cxx2b
    LABEL "C++2b support"
    CODE
"#if __cplusplus > 202002L
// Compiler claims to support C++2B, trust it
#else
#  error __cplusplus must be > 202002L (the value for C++20)
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
"
    CXX_STANDARD 23
)

# precompile_header
qt_config_compile_test(precompile_header
    LABEL "precompiled header support"
    PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/precompile_header"
)

qt_config_compiler_supports_flag_test(optimize_debug
    LABEL "-Og support"
    FLAG "-Og"
)

qt_config_compile_test(no_direct_extern_access
    LABEL "-mno-direct-extern-access / -fno-direct-access-external-data support"
    PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/no_direct_extern_access"
)

qt_config_linker_supports_flag_test(enable_new_dtags
    LABEL "new dtags support"
    FLAG "--enable-new-dtags"
)

qt_config_linker_supports_flag_test(gdb_index
    LABEL "gdb index support"
    FLAG "--gdb-index"
)

# reduce_relocations
qt_config_compile_test(reduce_relocations
    LABEL "-Bsymbolic-functions support"
    CODE
"#if !(defined(__i386) || defined(__i386__) || defined(__x86_64) || defined(__x86_64__) || defined(__amd64)) || defined(__sun)
#  error Symbolic function binding on this architecture may be broken, disabling it (see QTBUG-36129).
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
"# FIXME: qmake: ['TEMPLATE = lib', 'CONFIG += dll bsymbolic_functions', 'isEmpty(QMAKE_LFLAGS_BSYMBOLIC_FUNC): error("Nope")']
)

if(NOT MSVC AND NOT APPLE)
    qt_config_compile_test("separate_debug_info"
                       LABEL "separate debug information support"
                       PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/separate_debug_info"
    )
endif()

# signaling_nan
qt_config_compile_test(signaling_nan
    LABEL "Signaling NaN for doubles"
    CODE
"#if defined(__ghs)  && (__GHS_VERSION_NUMBER <= 202014)
#  error Signal NaNs are not supported by GHS compiler, but has_signaling_NaN returns TRUE. Will be fixed in future compiler releases.
#endif

#include <limits>

int main(void)
{
    /* BEGIN TEST: */
using B = std::numeric_limits<double>;
static_assert(B::has_signaling_NaN, \"System lacks signaling NaN\");
    /* END TEST: */
    return 0;
}
")

# basic x86 intrinsics support
qt_config_compile_test(x86intrin
    LABEL "Basic x86 intrinsics"
    PROJECT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/x86intrin"
)

# x86: avx512vbmi2
qt_config_compile_test_x86simd(avx512vbmi2 "AVX512VBMI2")

# x86: vaes
qt_config_compile_test_x86simd(vaes "VAES")

# posix_fallocate
qt_config_compile_test(posix_fallocate
    LABEL "POSIX fallocate()"
    CODE
"#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    /* BEGIN TEST: */
(void) posix_fallocate(0, 0, 0);
    /* END TEST: */
    return 0;
}
")

# alloca_stdlib_h
qt_config_compile_test(alloca_stdlib_h
    LABEL "alloca() in stdlib.h"
    CODE
"#include <stdlib.h>

int main(void)
{
    /* BEGIN TEST: */
alloca(1);
    /* END TEST: */
    return 0;
}
")

# alloca_h
qt_config_compile_test(alloca_h
    LABEL "alloca() in alloca.h"
    CODE
"#include <alloca.h>
#ifdef __QNXNTO__
// extra include needed in QNX7 to define NULL for the alloca() macro
#  include <stddef.h>
#endif

int main(void)
{
    /* BEGIN TEST: */
alloca(1);
    /* END TEST: */
    return 0;
}
")

# alloca_malloc_h
qt_config_compile_test(alloca_malloc_h
    LABEL "alloca() in malloc.h"
    CODE
"#include <malloc.h>

int main(void)
{
    /* BEGIN TEST: */
alloca(1);
    /* END TEST: */
    return 0;
}
")

# stack_protector
qt_config_compile_test(stack_protector
    LABEL "stack protection"
    COMPILE_OPTIONS -fstack-protector-strong
    CODE
"#ifdef __QNXNTO__
#  include <sys/neutrino.h>
#  if _NTO_VERSION < 700
#    error stack-protector not used (by default) before QNX 7.0.0.
#  endif
#endif

int main(void)
{
    /* BEGIN TEST: */
    /* END TEST: */
    return 0;
}
")

# intelcet
qt_config_compile_test(intelcet
    LABEL "Support for Intel Control-flow Enforcement Technology (CET)"
    CODE
"int main(void)
{
    /* BEGIN TEST: */
#if !defined(__CET__)
#  error Intel CET not available
#endif
    /* END TEST: */
    return 0;
}
")



#### Features

# This belongs into gui, but the license check needs it here already.
qt_feature("android-style-assets" PRIVATE
    LABEL "Android Style Assets"
    CONDITION ANDROID
)
qt_feature("shared" PUBLIC
    LABEL "Building shared libraries"
    CONDITION BUILD_SHARED_LIBS
)
qt_feature_definition("shared" "QT_STATIC" NEGATE PREREQUISITE "!defined(QT_SHARED) && !defined(QT_STATIC)")
qt_feature_config("shared" QMAKE_PUBLIC_QT_CONFIG)
qt_feature_config("shared" QMAKE_PUBLIC_CONFIG)
qt_feature("static" PUBLIC
    CONDITION NOT QT_FEATURE_shared
)
qt_feature_config("static" QMAKE_PUBLIC_QT_CONFIG)
qt_feature_config("static" QMAKE_PUBLIC_CONFIG)
qt_feature("cross_compile" PUBLIC
    LABEL "Cross compiling"
    CONDITION CMAKE_CROSSCOMPILING
)
qt_feature_config("cross_compile" QMAKE_PUBLIC_CONFIG)
qt_feature_config("cross_compile" QMAKE_PRIVATE_CONFIG)
qt_feature("gc_binaries" PRIVATE
    CONDITION NOT QT_FEATURE_shared
)
qt_feature("optimize_debug"
    LABEL "Optimize debug build"
    AUTODETECT NOT QT_FEATURE_developer_build
    CONDITION NOT MSVC AND NOT CLANG AND ( QT_FEATURE_debug OR QT_FEATURE_debug_and_release ) AND TEST_optimize_debug
)
qt_feature_config("optimize_debug" QMAKE_PRIVATE_CONFIG)
qt_feature("optimize_size"
    LABEL "Optimize release build for size"
    AUTODETECT OFF
    CONDITION NOT QT_FEATURE_debug OR QT_FEATURE_debug_and_release
)
qt_feature_config("optimize_size" QMAKE_PRIVATE_CONFIG)
qt_feature("optimize_full"
    LABEL "Fully optimize release builds (-O3)"
    AUTODETECT OFF
)
qt_feature_config("optimize_full" QMAKE_PRIVATE_CONFIG)
qt_feature("msvc_obj_debug_info"
    LABEL "Embed debug info in object files (MSVC)"
    CONDITION MSVC
    ENABLE QT_USE_CCACHE
    AUTODETECT OFF
)
qt_feature_config("msvc_obj_debug_info" QMAKE_PRIVATE_CONFIG)
qt_feature("pkg-config" PUBLIC
    LABEL "Using pkg-config"
    AUTODETECT NOT APPLE AND NOT WIN32 AND NOT ANDROID
    CONDITION PKG_CONFIG_FOUND
)
qt_feature_config("pkg-config" QMAKE_PUBLIC_QT_CONFIG
    NEGATE)
qt_feature("developer-build" PRIVATE
    LABEL "Developer build"
    AUTODETECT OFF
)
qt_feature("no-prefix"
    LABEL "No prefix build"
    AUTODETECT NOT QT_WILL_INSTALL
    CONDITION NOT QT_WILL_INSTALL
)
qt_feature("private_tests" PRIVATE
    LABEL "Developer build: private_tests"
    CONDITION QT_FEATURE_developer_build
)
qt_feature_definition("developer-build" "QT_BUILD_INTERNAL")
qt_feature_config("developer-build" QMAKE_PUBLIC_QT_CONFIG
    NAME "private_tests"
)
qt_feature("debug" PRIVATE
    LABEL "Build for debugging"
    AUTODETECT ON
    CONDITION CMAKE_BUILD_TYPE STREQUAL Debug OR Debug IN_LIST CMAKE_CONFIGURATION_TYPES
)
qt_feature("debug_and_release" PUBLIC
    LABEL "Compile libs in debug and release mode"
    AUTODETECT 1
    CONDITION QT_GENERATOR_IS_MULTI_CONFIG
)
qt_feature_config("debug_and_release" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("force_debug_info"
    LABEL "Add debug info in release mode"
    AUTODETECT CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo OR RelWithDebInfo IN_LIST CMAKE_CONFIGURATION_TYPES
)
qt_feature_config("force_debug_info" QMAKE_PRIVATE_CONFIG)
qt_feature("separate_debug_info" PUBLIC
    LABEL "Split off debug information"
    AUTODETECT OFF
    CONDITION ( QT_FEATURE_shared ) AND ( QT_FEATURE_debug OR QT_FEATURE_debug_and_release OR QT_FEATURE_force_debug_info ) AND ( MSVC OR APPLE OR TEST_separate_debug_info )
)
qt_feature_config("separate_debug_info" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("appstore-compliant" PUBLIC
    LABEL "App store compliance"
    PURPOSE "Disables code that is not allowed in platform app stores"
    AUTODETECT UIKIT OR ANDROID
)
if(APPLE)
    qt_feature_definition("appstore-compliant" "QT_APPLE_NO_PRIVATE_APIS")
endif()
qt_feature("simulator_and_device" PUBLIC
    LABEL "Build for both simulator and device"
    CONDITION UIKIT AND NOT QT_UIKIT_SDK
)
qt_feature_config("simulator_and_device" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("rpath" PUBLIC
    LABEL "Build with RPATH"
    AUTODETECT 1
    CONDITION BUILD_SHARED_LIBS AND UNIX AND NOT WIN32 AND NOT ANDROID
)
qt_feature_config("rpath" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("force_asserts" PUBLIC
    LABEL "Force assertions"
    AUTODETECT OFF
)
qt_feature("headersclean"
    LABEL "Check for clean headers"
    AUTODETECT OFF
    CONDITION NOT WASM
)
qt_feature_config("headersclean" QMAKE_PRIVATE_CONFIG)
qt_feature("framework" PUBLIC
    LABEL "Build Apple Frameworks"
    CONDITION APPLE AND BUILD_SHARED_LIBS AND NOT CMAKE_BUILD_TYPE STREQUAL Debug
)
qt_feature_definition("framework" "QT_MAC_FRAMEWORK_BUILD")
qt_feature_config("framework" QMAKE_PUBLIC_QT_CONFIG
    NAME "qt_framework"
)
qt_feature_config("framework" QMAKE_PUBLIC_CONFIG
    NAME "qt_framework"
)
qt_feature("largefile"
    LABEL "Large file support"
    CONDITION NOT ANDROID AND NOT INTEGRITY AND NOT rtems
)
qt_feature_definition("largefile" "QT_LARGEFILE_SUPPORT" VALUE "64")
qt_feature_config("largefile" QMAKE_PRIVATE_CONFIG)
qt_feature("testcocoon"
    LABEL "Testcocoon support"
    AUTODETECT OFF
)
qt_feature_config("testcocoon" QMAKE_PUBLIC_CONFIG)
qt_feature("sanitize_address"
    LABEL "Addresses"
    AUTODETECT OFF
)
qt_feature_config("sanitize_address" QMAKE_PUBLIC_CONFIG)
qt_feature("sanitize_thread"
    LABEL "Threads"
    AUTODETECT OFF
)
qt_feature_config("sanitize_thread" QMAKE_PUBLIC_CONFIG)
qt_feature("sanitize_memory"
    LABEL "Memory"
    AUTODETECT OFF
)
qt_feature_config("sanitize_memory" QMAKE_PUBLIC_CONFIG)
qt_feature("sanitize_fuzzer_no_link"
    LABEL "Fuzzer (instrumentation only)"
    PURPOSE "Adds instrumentation for fuzzing to the binaries but links to the usual main function instead of a fuzzer's."
    AUTODETECT OFF
)
qt_feature_config("sanitize_fuzzer_no_link" QMAKE_PUBLIC_CONFIG)
qt_feature("sanitize_undefined"
    LABEL "Undefined"
    AUTODETECT OFF
)
qt_feature_config("sanitize_undefined" QMAKE_PUBLIC_CONFIG)
qt_feature("sanitizer"
    LABEL "Sanitizers"
    CONDITION QT_FEATURE_sanitize_address OR QT_FEATURE_sanitize_thread OR QT_FEATURE_sanitize_memory OR QT_FEATURE_sanitize_fuzzer_no_link OR QT_FEATURE_sanitize_undefined
)
qt_feature_config("sanitizer" QMAKE_PUBLIC_CONFIG)
qt_feature("plugin-manifests"
    LABEL "Embed manifests in plugins"
    AUTODETECT OFF
    EMIT_IF WIN32
)
qt_feature_config("plugin-manifests" QMAKE_PUBLIC_CONFIG
    NEGATE
    NAME "no_plugin_manifest"
)
qt_feature("c++20" PUBLIC
    LABEL "C++20"
    AUTODETECT OFF
    CONDITION TEST_cxx20
)
qt_feature_config("c++20" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("c++2a" PUBLIC
    LABEL "C++20"
    CONDITION QT_FEATURE_cxx20
)
qt_feature_config("c++2a" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("c++2b" PUBLIC
    LABEL "C++2b"
    AUTODETECT OFF
)
qt_feature_config("c++2b" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("c++2b" PUBLIC
    LABEL "C++2b"
    AUTODETECT FALSE
    CONDITION QT_FEATURE_cxx20 AND (CMAKE_VERSION VERSION_GREATER_EQUAL "3.20") AND TEST_cxx2b
)
qt_feature("precompile_header"
    LABEL "Using precompiled headers"
    CONDITION BUILD_WITH_PCH AND TEST_precompile_header
    AUTODETECT NOT WASM
)
qt_feature_config("precompile_header" QMAKE_PRIVATE_CONFIG)
set(__qt_ltcg_detected FALSE)
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(__qt_ltcg_detected TRUE)
else()
    foreach(config ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${config}" __qt_uc_config)
        if(CMAKE_INTERPROCEDURAL_OPTIMIZATION_${__qt_uc_config})
            set(__qt_ltcg_detected TRUE)
            break()
        endif()
    endforeach()
    unset(__qt_uc_config)
endif()
qt_feature("ltcg"
    LABEL "Using Link Time Optimization (LTCG)"
    AUTODETECT ON
    CONDITION __qt_ltcg_detected
)
qt_feature_config("ltcg" QMAKE_PRIVATE_CONFIG)

if(NOT QT_CONFIGURE_RUNNING)
    # This feature is used early in QtCompilerOptimization.cmake.
    qt_evaluate_feature(ltcg)
endif()

qt_feature("enable_new_dtags"
    LABEL "Using new DTAGS"
    CONDITION LINUX AND TEST_enable_new_dtags
)
qt_feature_config("enable_new_dtags" QMAKE_PRIVATE_CONFIG)
qt_feature("enable_gdb_index"
    LABEL "Generating GDB index"
    AUTODETECT QT_FEATURE_developer_build
    CONDITION GCC AND NOT CLANG AND ( QT_FEATURE_debug OR QT_FEATURE_force_debug_info OR QT_FEATURE_debug_and_release ) AND TEST_gdb_index
)
qt_feature_config("enable_gdb_index" QMAKE_PRIVATE_CONFIG)
qt_feature("reduce_exports" PRIVATE
    LABEL "Reduce amount of exported symbols"
    CONDITION NOT MSVC
)
qt_feature_definition("reduce_exports" "QT_VISIBILITY_AVAILABLE")
qt_feature_config("reduce_exports" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("no_direct_extern_access" PRIVATE
    LABEL "Use protected visibility and -mno-direct-extern-access"
    CONDITION NOT WIN32 AND TEST_no_direct_extern_access
    AUTODETECT OFF
)
qt_feature_definition("no_direct_extern_access" "QT_USE_PROTECTED_VISIBILITY")
qt_feature_config("no_direct_extern_access" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("reduce_relocations" PUBLIC
    LABEL "Reduce amount of relocations"
    CONDITION NOT WIN32 AND TEST_reduce_relocations
)
qt_feature_definition("reduce_relocations" "QT_REDUCE_RELOCATIONS")
qt_feature_config("reduce_relocations" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("signaling_nan" PUBLIC
    LABEL "Signaling NaN"
    CONDITION TEST_signaling_nan
)
qt_feature("x86intrin" PRIVATE
    LABEL "Basic"
    CONDITION (((TEST_architecture_arch STREQUAL i386) OR (TEST_architecture_arch STREQUAL x86_64))
        AND (QT_FORCE_FEATURE_x86intrin OR TEST_x86intrin))
    AUTODETECT NOT WASM
)
qt_feature("sse2" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("sse2" "QT_COMPILER_SUPPORTS_SSE2" VALUE "1")
qt_feature_config("sse2" QMAKE_PRIVATE_CONFIG)
qt_feature("sse3" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("sse3" "QT_COMPILER_SUPPORTS_SSE3" VALUE "1")
qt_feature_config("sse3" QMAKE_PRIVATE_CONFIG)
qt_feature("ssse3" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("ssse3" "QT_COMPILER_SUPPORTS_SSSE3" VALUE "1")
qt_feature_config("ssse3" QMAKE_PRIVATE_CONFIG)
qt_feature("sse4_1" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("sse4_1" "QT_COMPILER_SUPPORTS_SSE4_1" VALUE "1")
qt_feature_config("sse4_1" QMAKE_PRIVATE_CONFIG)
qt_feature("sse4_2" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("sse4_2" "QT_COMPILER_SUPPORTS_SSE4_2" VALUE "1")
qt_feature_config("sse4_2" QMAKE_PRIVATE_CONFIG)
qt_feature("avx" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx" "QT_COMPILER_SUPPORTS_AVX" VALUE "1")
qt_feature_config("avx" QMAKE_PRIVATE_CONFIG)
qt_feature("f16c" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("f16c" "QT_COMPILER_SUPPORTS_F16C" VALUE "1")
qt_feature_config("f16c" QMAKE_PRIVATE_CONFIG)
qt_feature("avx2" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx2" "QT_COMPILER_SUPPORTS_AVX2" VALUE "1")
qt_feature_config("avx2" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512f" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512f" "QT_COMPILER_SUPPORTS_AVX512F" VALUE "1")
qt_feature_config("avx512f" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512er" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512er" "QT_COMPILER_SUPPORTS_AVX512ER" VALUE "1")
qt_feature_config("avx512er" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512cd" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512cd" "QT_COMPILER_SUPPORTS_AVX512CD" VALUE "1")
qt_feature_config("avx512cd" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512pf" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512pf" "QT_COMPILER_SUPPORTS_AVX512PF" VALUE "1")
qt_feature_config("avx512pf" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512dq" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512dq" "QT_COMPILER_SUPPORTS_AVX512DQ" VALUE "1")
qt_feature_config("avx512dq" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512bw" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512bw" "QT_COMPILER_SUPPORTS_AVX512BW" VALUE "1")
qt_feature_config("avx512bw" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512vl" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512vl" "QT_COMPILER_SUPPORTS_AVX512VL" VALUE "1")
qt_feature_config("avx512vl" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512ifma" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512ifma" "QT_COMPILER_SUPPORTS_AVX512IFMA" VALUE "1")
qt_feature_config("avx512ifma" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512vbmi" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("avx512vbmi" "QT_COMPILER_SUPPORTS_AVX512VBMI" VALUE "1")
qt_feature_config("avx512vbmi" QMAKE_PRIVATE_CONFIG)
qt_feature("avx512vbmi2" PRIVATE
    LABEL "AVX512VBMI2"
    CONDITION QT_FEATURE_x86intrin AND TEST_subarch_avx512vbmi2
)
qt_feature_definition("avx512vbmi2" "QT_COMPILER_SUPPORTS_AVX512VBMI2" VALUE "1")
qt_feature_config("avx512vbmi2" QMAKE_PRIVATE_CONFIG)
qt_feature("aesni" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("aesni" "QT_COMPILER_SUPPORTS_AES" VALUE "1")
qt_feature_config("aesni" QMAKE_PRIVATE_CONFIG)
qt_feature("vaes" PRIVATE
    LABEL "VAES"
    CONDITION QT_FEATURE_x86intrin AND TEST_subarch_vaes
)
qt_feature_definition("vaes" "QT_COMPILER_SUPPORTS_VAES" VALUE "1")
qt_feature_config("vaes" QMAKE_PRIVATE_CONFIG)
qt_feature("rdrnd" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("rdrnd" "QT_COMPILER_SUPPORTS_RDRND" VALUE "1")
qt_feature_config("rdrnd" QMAKE_PRIVATE_CONFIG)
qt_feature("rdseed" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("rdseed" "QT_COMPILER_SUPPORTS_RDSEED" VALUE "1")
qt_feature_config("rdseed" QMAKE_PRIVATE_CONFIG)
qt_feature("shani" PRIVATE
    CONDITION QT_FEATURE_x86intrin
)
qt_feature_definition("shani" "QT_COMPILER_SUPPORTS_SHA" VALUE "1")
qt_feature_config("shani" QMAKE_PRIVATE_CONFIG)
qt_feature("mips_dsp" PRIVATE
    LABEL "DSP"
    CONDITION ( TEST_architecture_arch STREQUAL mips ) AND TEST_arch_${TEST_architecture_arch}_subarch_dsp
)
qt_feature_definition("mips_dsp" "QT_COMPILER_SUPPORTS_MIPS_DSP" VALUE "1")
qt_feature_config("mips_dsp" QMAKE_PRIVATE_CONFIG)
qt_feature("mips_dspr2" PRIVATE
    LABEL "DSPr2"
    CONDITION ( TEST_architecture_arch STREQUAL mips ) AND TEST_arch_${TEST_architecture_arch}_subarch_dspr2
)
qt_feature_definition("mips_dspr2" "QT_COMPILER_SUPPORTS_MIPS_DSPR2" VALUE "1")
qt_feature_config("mips_dspr2" QMAKE_PRIVATE_CONFIG)
qt_feature("neon" PRIVATE
    LABEL "NEON"
    CONDITION ( ( ( TEST_architecture_arch STREQUAL arm ) OR
        ( TEST_architecture_arch STREQUAL arm64 ) ) AND
        TEST_arch_${TEST_architecture_arch}_subarch_neon ) OR QT_FORCE_FEATURE_neon
)
qt_feature_definition("neon" "QT_COMPILER_SUPPORTS_NEON" VALUE "1")
qt_feature_config("neon" QMAKE_PRIVATE_CONFIG)
qt_feature("arm_crc32" PRIVATE
    LABEL "CRC32"
    CONDITION ( ( TEST_architecture_arch STREQUAL arm ) OR ( TEST_architecture_arch STREQUAL arm64 ) ) AND TEST_arch_${TEST_architecture_arch}_subarch_crc32
)
qt_feature_definition("arm_crc32" "QT_COMPILER_SUPPORTS_CRC32" VALUE "1")
qt_feature_config("arm_crc32" QMAKE_PRIVATE_CONFIG)
qt_feature("arm_crypto" PRIVATE
    LABEL "AES"
    CONDITION ( ( TEST_architecture_arch STREQUAL arm ) OR ( TEST_architecture_arch STREQUAL arm64 ) ) AND TEST_arch_${TEST_architecture_arch}_subarch_crypto
)
qt_feature_definition("arm_crypto" "QT_COMPILER_SUPPORTS_AES" VALUE "1")
qt_feature_config("arm_crypto" QMAKE_PRIVATE_CONFIG)

qt_feature("wasm-simd128" PUBLIC
    LABEL "WebAssembly SIMD128"
    PURPOSE "Enables WebAssembly SIMD"
    AUTODETECT OFF
)
qt_feature_definition("wasm-simd128" "QT_COMPILER_SUPPORTS_WASM_SIMD128" VALUE "1")
qt_feature_config("wasm-simd128" QMAKE_PRIVATE_CONFIG)

qt_feature("wasm-exceptions" PUBLIC
    LABEL "WebAssembly Exceptions"
    PURPOSE "Enables WebAssembly Exceptions"
    AUTODETECT OFF
)
qt_feature_definition("wasm-exceptions" "QT_WASM_EXCEPTIONS" VALUE "1")
qt_feature_config("wasm-exceptions" QMAKE_PRIVATE_CONFIG)

qt_feature("posix_fallocate" PRIVATE
    LABEL "POSIX fallocate()"
    CONDITION TEST_posix_fallocate
)
qt_feature("alloca_h" PRIVATE
    LABEL "alloca.h"
    CONDITION TEST_alloca_h
)
qt_feature("alloca_malloc_h" PRIVATE
    LABEL "alloca() in malloc.h"
    CONDITION NOT QT_FEATURE_alloca_h AND TEST_alloca_malloc_h
)
qt_feature("alloca" PRIVATE
    LABEL "alloca()"
    CONDITION QT_FEATURE_alloca_h OR QT_FEATURE_alloca_malloc_h OR TEST_alloca_stdlib_h
)
qt_feature("stack-protector-strong" PRIVATE
    LABEL "stack protection"
    CONDITION QNX AND TEST_stack_protector
)
qt_feature("system-zlib" PRIVATE
    LABEL "Using system zlib"
    CONDITION WrapSystemZLIB_FOUND
)
qt_feature("zstd" PUBLIC
    LABEL "Zstandard support"
    CONDITION WrapZSTD_FOUND
)
qt_feature("stdlib-libcpp" PRIVATE
    LABEL "Using stdlib=libc++"
    AUTODETECT OFF
    CONDITION LINUX AND NOT ANDROID
)
# Check whether CMake was built with zstd support.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/21552
if(NOT DEFINED CACHE{QT_CMAKE_ZSTD_SUPPORT})
    set(QT_CMAKE_ZSTD_SUPPORT FALSE CACHE INTERNAL "")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        execute_process(COMMAND "${CMAKE_COMMAND}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/cmake_zstd/check_zstd.cmake"
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/config.tests"
            OUTPUT_QUIET ERROR_QUIET
            RESULT_VARIABLE qt_check_zstd_exit_code)
        if(qt_check_zstd_exit_code EQUAL 0)
            set(QT_CMAKE_ZSTD_SUPPORT TRUE CACHE INTERNAL "")
        endif()
        unset(qt_check_zstd_exit_code)
    endif()
endif()
qt_feature("thread" PUBLIC
    SECTION "Kernel"
    LABEL "Thread support"
    PURPOSE "Provides QThread and related classes."
    AUTODETECT NOT WASM
)
qt_feature("future" PUBLIC
    SECTION "Kernel"
    LABEL "QFuture"
    PURPOSE "Provides QFuture and related classes."
    CONDITION QT_FEATURE_thread
)
qt_feature("concurrent" PUBLIC
    SECTION "Kernel"
    LABEL "Qt Concurrent"
    PURPOSE "Provides a high-level multi-threading API."
    CONDITION QT_FEATURE_future
)
qt_feature_definition("concurrent" "QT_NO_CONCURRENT" NEGATE VALUE "1")
qt_feature("dbus" PUBLIC PRIVATE
    LABEL "Qt D-Bus"
    AUTODETECT NOT UIKIT AND NOT ANDROID
    CONDITION QT_FEATURE_thread AND NOT WASM
)
qt_feature_definition("dbus" "QT_NO_DBUS" NEGATE VALUE "1")
qt_feature("dbus-linked" PRIVATE
    LABEL "Qt D-Bus directly linked to libdbus"
    CONDITION QT_FEATURE_dbus AND DBus1_FOUND
    ENABLE INPUT_dbus STREQUAL 'linked'
    DISABLE INPUT_dbus STREQUAL 'runtime'
)
qt_feature("qreal"
    LABEL "Type for qreal"
    CONDITION DEFINED QT_COORD_TYPE AND NOT QT_COORD_TYPE STREQUAL "double"
)
qt_feature_definition("qreal" "QT_COORD_TYPE" VALUE "${QT_COORD_TYPE}")
qt_feature_definition("qreal" "QT_COORD_TYPE_STRING" VALUE "\"${QT_COORD_TYPE}\"")
qt_feature("gui" PRIVATE
    LABEL "Qt Gui"
)
qt_feature_config("gui" QMAKE_PUBLIC_QT_CONFIG
    NEGATE)
qt_feature("network" PRIVATE
    LABEL "Qt Network"
)
qt_feature("printsupport" PRIVATE
    LABEL "Qt PrintSupport"
    CONDITION QT_FEATURE_widgets
)
qt_feature("sql" PRIVATE
    LABEL "Qt Sql"
)
qt_feature("testlib" PRIVATE
    LABEL "Qt Testlib"
)
qt_feature("widgets" PRIVATE
    LABEL "Qt Widgets"
    AUTODETECT NOT TVOS AND NOT WATCHOS
    CONDITION QT_FEATURE_gui
)
qt_feature_definition("widgets" "QT_NO_WIDGETS" NEGATE)
qt_feature_config("widgets" QMAKE_PUBLIC_QT_CONFIG
    NEGATE)
qt_feature("xml" PRIVATE
    LABEL "Qt Xml"
)
qt_feature("libudev" PRIVATE
    LABEL "udev"
    CONDITION Libudev_FOUND AND NOT INTEGRITY
)
qt_feature("openssl" PRIVATE
    LABEL "OpenSSL"
    CONDITION QT_FEATURE_openssl_runtime OR QT_FEATURE_openssl_linked
    ENABLE false
)
qt_feature_definition("openssl" "QT_NO_OPENSSL" NEGATE)
qt_feature_config("openssl" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("openssl-runtime"
    AUTODETECT NOT WASM
    CONDITION TEST_opensslv11_headers OR TEST_opensslv30_headers
    ENABLE INPUT_openssl STREQUAL 'yes' OR INPUT_openssl STREQUAL 'runtime'
    DISABLE INPUT_openssl STREQUAL 'no' OR INPUT_openssl STREQUAL 'linked' OR INPUT_ssl STREQUAL 'no'
)
qt_feature("openssl-linked" PUBLIC
    LABEL "  Qt directly linked to OpenSSL"
    AUTODETECT OFF
    CONDITION TEST_opensslv11 OR TEST_opensslv30
    ENABLE INPUT_openssl STREQUAL 'linked'
)
qt_feature_definition("openssl-linked" "QT_LINKED_OPENSSL")
qt_feature("opensslv11" PUBLIC
    LABEL "OpenSSL 1.1"
    CONDITION TEST_opensslv11 OR TEST_opensslv11_headers
    DISABLE INPUT_openssl STREQUAL 'no' OR INPUT_ssl STREQUAL 'no'
)
qt_feature("opensslv30" PUBLIC
    LABEL "OpenSSL 3.0"
    CONDITION TEST_opensslv30 OR TEST_opensslv30_headers
    DISABLE INPUT_openssl STREQUAL 'no' OR INPUT_ssl STREQUAL 'no'
)
qt_feature("ccache"
    LABEL "Using ccache"
    AUTODETECT 1
    CONDITION QT_USE_CCACHE
)
qt_feature_config("ccache" QMAKE_PRIVATE_CONFIG)
qt_feature("static_runtime"
    LABEL "Statically link the C/C++ runtime library"
    AUTODETECT OFF
    CONDITION NOT QT_FEATURE_shared
    EMIT_IF WIN32
)
qt_feature_config("static_runtime" QMAKE_PUBLIC_CONFIG)
qt_feature_config("static_runtime" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("dlopen" PRIVATE
    LABEL "dlopen()"
    CONDITION UNIX AND NOT INTEGRITY
)
qt_feature("relocatable" PRIVATE
    LABEL "Relocatable"
    PURPOSE "Enable the Qt installation to be relocated."
    AUTODETECT QT_FEATURE_shared
    CONDITION QT_FEATURE_dlopen OR WIN32 OR NOT QT_FEATURE_shared
)
qt_feature("intelcet" PRIVATE
    LABEL "Using Intel CET"
    CONDITION ( INPUT_intelcet STREQUAL yes ) OR TEST_intelcet
)
qt_configure_add_summary_build_type_and_config()
qt_configure_add_summary_section(NAME "Build options")
qt_configure_add_summary_build_mode(Mode)
qt_configure_add_summary_entry(
    ARGS "optimize_debug"
    CONDITION NOT MSVC AND NOT CLANG AND ( QT_FEATURE_debug OR QT_FEATURE_debug_and_release )
)
qt_configure_add_summary_entry(
    ARGS "optimize_size"
    CONDITION NOT QT_FEATURE_debug OR QT_FEATURE_debug_and_release
)
qt_configure_add_summary_entry(
    ARGS "optimize_full"
)
qt_configure_add_summary_entry(ARGS "shared")
qt_configure_add_summary_entry(
    ARGS "ccache"
    CONDITION UNIX
)
qt_configure_add_summary_entry(
    TYPE "message" ARGS "Unity Build" MESSAGE "yes" CONDITION QT_UNITY_BUILD
)
qt_configure_add_summary_entry(
    TYPE "message" ARGS "Unity Build" MESSAGE "no" CONDITION NOT QT_UNITY_BUILD
)
qt_configure_add_summary_entry(
    TYPE "message"
    ARGS "Unity Build Batch Size"
    MESSAGE "${QT_UNITY_BUILD_BATCH_SIZE}"
    CONDITION QT_UNITY_BUILD
)
qt_configure_add_summary_entry(
    TYPE "firstAvailableFeature"
    ARGS "use_bfd_linker use_gold_linker use_lld_linker use_mold_linker"
    MESSAGE "Linker"
    CONDITION QT_FEATURE_use_bfd_linker OR QT_FEATURE_use_gold_linker OR QT_FEATURE_use_lld_linker
              OR QT_FEATURE_use_mold_linker
)
qt_configure_add_summary_entry(
    ARGS "enable_new_dtags"
    CONDITION LINUX
)
qt_configure_add_summary_entry(
    ARGS "enable_gdb_index"
    CONDITION GCC AND NOT CLANG AND ( QT_FEATURE_debug OR QT_FEATURE_force_debug_info OR QT_FEATURE_debug_and_release )
)
qt_configure_add_summary_entry(ARGS "relocatable")
qt_configure_add_summary_entry(ARGS "precompile_header")
qt_configure_add_summary_entry(ARGS "ltcg")
qt_configure_add_summary_entry(ARGS "intelcet")
qt_configure_add_summary_entry(
    ARGS "wasm-simd128"
    CONDITION ( TEST_architecture_arch STREQUAL wasm )
)
qt_configure_add_summary_entry(
    ARGS "wasm-exceptions"
    CONDITION ( TEST_architecture_arch STREQUAL wasm )
)
qt_configure_add_summary_section(NAME "Target compiler supports")
qt_configure_add_summary_entry(
    TYPE "featureList"
    ARGS "x86intrin vaes avx512vbmi2"
    MESSAGE "x86 Intrinsics"
    CONDITION ( ( TEST_architecture_arch STREQUAL i386 ) OR ( TEST_architecture_arch STREQUAL x86_64 ) )
)
qt_configure_add_summary_entry(
    TYPE "featureList"
    ARGS "neon arm_crc32 arm_crypto"
    MESSAGE "ARM Extensions"
    CONDITION ( TEST_architecture_arch STREQUAL arm ) OR ( TEST_architecture_arch STREQUAL arm64 )
)
qt_configure_add_summary_entry(
    ARGS "mips_dsp"
    CONDITION ( TEST_architecture_arch STREQUAL mips )
)
qt_configure_add_summary_entry(
    ARGS "mips_dspr2"
    CONDITION ( TEST_architecture_arch STREQUAL mips )
)
qt_configure_end_summary_section() # end of "Target compiler supports" section
qt_configure_add_summary_section(NAME "Sanitizers")
qt_configure_add_summary_entry(ARGS "sanitize_address")
qt_configure_add_summary_entry(ARGS "sanitize_thread")
qt_configure_add_summary_entry(ARGS "sanitize_memory")
qt_configure_add_summary_entry(ARGS "sanitize_fuzzer_no_link")
qt_configure_add_summary_entry(ARGS "sanitize_undefined")
qt_configure_end_summary_section() # end of "Sanitizers" section
qt_configure_add_summary_build_parts("Build parts")
if(QT_INSTALL_EXAMPLES_SOURCES)
    set(_examples_sources_entry_message "yes")
else()
    set(_examples_sources_entry_message "no")
endif()
qt_configure_add_summary_entry(ARGS "Install examples sources" TYPE "message"
    MESSAGE "${_examples_sources_entry_message}")
qt_configure_add_summary_entry(
    ARGS "appstore-compliant"
    CONDITION APPLE OR ANDROID OR WIN32
)
qt_configure_end_summary_section() # end of "Build options" section
qt_configure_add_summary_section(NAME "Qt modules and options")
qt_configure_add_summary_entry(ARGS "concurrent")
qt_configure_add_summary_entry(ARGS "dbus")
qt_configure_add_summary_entry(ARGS "dbus-linked")
qt_configure_add_summary_entry(ARGS "gui")
qt_configure_add_summary_entry(ARGS "network")
qt_configure_add_summary_entry(ARGS "printsupport")
qt_configure_add_summary_entry(ARGS "sql")
qt_configure_add_summary_entry(ARGS "testlib")
qt_configure_add_summary_entry(ARGS "widgets")
qt_configure_add_summary_entry(ARGS "xml")
qt_configure_end_summary_section() # end of "Qt modules and options" section
qt_configure_add_summary_section(NAME "Support enabled for")
qt_configure_add_summary_entry(ARGS "pkg-config")

if(QT_USE_VCPKG AND (DEFINED ENV{VCPKG_ROOT} OR VCPKG_TARGET_TRIPLET))
    set(_vcpkg_entry_message "yes")
else()
    set(_vcpkg_entry_message "no")
endif()
qt_configure_add_summary_entry(ARGS "Using vcpkg" TYPE "message" MESSAGE "${_vcpkg_entry_message}")

qt_configure_add_summary_entry(ARGS "libudev")
qt_configure_add_summary_entry(ARGS "openssl")
qt_configure_add_summary_entry(ARGS "openssl-linked")
qt_configure_add_summary_entry(ARGS "opensslv11")
qt_configure_add_summary_entry(ARGS "opensslv30")
qt_configure_add_summary_entry(ARGS "system-zlib")
qt_configure_add_summary_entry(ARGS "zstd")
qt_configure_add_summary_entry(ARGS "thread")
qt_configure_end_summary_section() # end of "Support enabled for" section
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "Using static linking will disable the use of dynamically loaded plugins. Make sure to import all needed static plugins, or compile needed modules into the library."
    CONDITION NOT QT_FEATURE_shared
)
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "-debug-and-release is only supported on Darwin and Windows platforms.  Qt can be built in release mode with separate debug information, so -debug-and-release is no longer necessary."
    CONDITION INPUT_debug_and_release STREQUAL 'yes' AND NOT APPLE AND NOT WIN32
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "debug-only framework builds are not supported. Configure with -no-framework if you want a pure debug build."
    CONDITION QT_FEATURE_framework AND QT_FEATURE_debug AND NOT QT_FEATURE_debug_and_release
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "Static builds don't support RPATH"
    CONDITION ( QT_FEATURE_rpath OR QT_EXTRA_RPATHS ) AND NOT QT_FEATURE_shared
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "Command line option -sanitize fuzzer-no-link is only supported with clang compilers."
    CONDITION QT_FEATURE_sanitize_fuzzer_no_link AND NOT CLANG
)
if (TEST_architecture_arch STREQUAL x86_64 OR TEST_architecture_arch STREQUAL i386)
    if ((TEST_architecture_arch STREQUAL i386) OR QNX OR WASM)
        # Warn only
        qt_configure_add_report_entry(
            TYPE WARNING
            CONDITION (NOT QT_FEATURE_x86intrin)
            MESSAGE [=[
All x86 intrinsics and SIMD support were disabled. If this was in error, check
the result of the build in config.tests/x86intrin and report at https://bugreports.qt.io.
]=]
        )
    elseif (MSVC AND CLANG)
        # Warn only
        qt_configure_add_report_entry(
            TYPE WARNING
            CONDITION (NOT QT_FEATURE_x86intrin)
            MESSAGE [=[
x86 intrinsics support is disabled for clang-cl build. This might be necessary due to
https://github.com/llvm/llvm-project/issues/53520
]=]
        )
    else()
        qt_configure_add_report_entry(
            TYPE ERROR
            CONDITION (NOT QT_FEATURE_x86intrin)
            MESSAGE [========[
x86 intrinsics support missing. Check your compiler settings. If this is an
error, report at https://bugreports.qt.io with your compiler ID and version,
and this output:

${TEST_x86intrin_OUTPUT}
]========]
        )
    endif()
endif()
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "Setting a library infix is not supported for framework builds."
    CONDITION QT_FEATURE_framework AND DEFINED QT_LIBINFIX
)
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "Enable thread support"
    CONDITION QT_FEATURE_thread AND WASM
)
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "You should use the recommended Emscripten version ${QT_EMCC_RECOMMENDED_VERSION} with this Qt. You have ${EMCC_VERSION}."
    CONDITION WASM AND NOT ${EMCC_VERSION} MATCHES ${QT_EMCC_RECOMMENDED_VERSION}
)
if(WASM)
    qt_extra_definition("QT_EMCC_VERSION" "\"${EMCC_VERSION}\"" PUBLIC)
endif()
qt_extra_definition("QT_VERSION_STR" "\"${PROJECT_VERSION}\"" PUBLIC)
qt_extra_definition("QT_VERSION_MAJOR" ${PROJECT_VERSION_MAJOR} PUBLIC)
qt_extra_definition("QT_VERSION_MINOR" ${PROJECT_VERSION_MINOR} PUBLIC)
qt_extra_definition("QT_VERSION_PATCH" ${PROJECT_VERSION_PATCH} PUBLIC)

qt_extra_definition("QT_COPYRIGHT" \"${QT_COPYRIGHT}\" PRIVATE)
qt_extra_definition("QT_COPYRIGHT_YEAR" \"${QT_COPYRIGHT_YEAR}\" PRIVATE)

qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "QT_ALLOW_SYMLINK_IN_PATHS is enabled. This is not recommended, and it may lead to unexpected issues.
E.g., When building QtWebEngine, enabling this option may result in build issues in certain platforms.
See https://bugreports.qt.io/browse/QTBUG-59769."
    CONDITION QT_ALLOW_SYMLINK_IN_PATHS
)
