#####################################################################
## BundledPcre2 Generic Library:
#####################################################################

qt_internal_add_3rdparty_library(BundledPcre2
    QMAKE_LIB_NAME pcre2
    STATIC
    SKIP_AUTOMOC
    SOURCES
        src/config.h
        src/pcre2.h
        src/pcre2_auto_possess.c
        src/pcre2_chartables.c
        src/pcre2_compile.c
        src/pcre2_config.c
        src/pcre2_context.c
        src/pcre2_dfa_match.c
        src/pcre2_error.c
        src/pcre2_extuni.c
        src/pcre2_find_bracket.c
        src/pcre2_internal.h
        src/pcre2_intmodedep.h
        src/pcre2_jit_compile.c
        src/pcre2_maketables.c
        src/pcre2_match.c
        src/pcre2_match_data.c
        src/pcre2_newline.c
        src/pcre2_ord2utf.c
        src/pcre2_pattern_info.c
        src/pcre2_script_run.c
        src/pcre2_serialize.c
        src/pcre2_string_utils.c
        src/pcre2_study.c
        src/pcre2_substitute.c
        src/pcre2_substring.c
        src/pcre2_tables.c
        src/pcre2_ucd.c
        src/pcre2_ucp.h
        src/pcre2_valid_utf.c
        src/pcre2_xclass.c
    DEFINES
        HAVE_CONFIG_H
    PUBLIC_DEFINES
        PCRE2_CODE_UNIT_WIDTH=16
    PUBLIC_INCLUDE_DIRECTORIES
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)
qt_disable_warnings(BundledPcre2)
qt_set_symbol_visibility_hidden(BundledPcre2)

## Scopes:
#####################################################################

qt_internal_extend_target(BundledPcre2 CONDITION QNX OR UIKIT
    DEFINES
        PCRE2_DISABLE_JIT
)

qt_internal_extend_target(BundledPcre2 CONDITION (TEST_architecture_arch STREQUAL "arm") AND WIN32
    DEFINES
        PCRE2_DISABLE_JIT
)

qt_internal_extend_target(BundledPcre2 CONDITION (TEST_architecture_arch STREQUAL "arm64") AND WIN32
    DEFINES
        PCRE2_DISABLE_JIT
)

if (APPLE)
    target_compile_options(BundledPcre2 PRIVATE "SHELL:-Xarch_arm64 -DPCRE2_DISABLE_JIT")
endif()

qt_internal_extend_target(BundledPcre2 CONDITION WIN32
    PUBLIC_DEFINES
        PCRE2_STATIC
)

qt_internal_apply_intel_cet(BundledPcre2 PRIVATE)
