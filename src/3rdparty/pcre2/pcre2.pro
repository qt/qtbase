TARGET = qtpcre2

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off


MODULE_DEFINES += PCRE2_CODE_UNIT_WIDTH=16
win32: MODULE_DEFINES += PCRE2_STATIC
MODULE_INCLUDEPATH += $$PWD/src

load(qt_helper_lib)

DEFINES += HAVE_CONFIG_H

qtConfig(intelcet) {
    QMAKE_CFLAGS += $$QMAKE_CFLAGS_SHSTK
    QMAKE_CXXFLAGS += $$QMAKE_CXXFLAGS_SHSTK
}

# platform/compiler specific definitions
uikit|qnx|winrt: DEFINES += PCRE2_DISABLE_JIT
win32:contains(QT_ARCH, "arm"): DEFINES += PCRE2_DISABLE_JIT
win32:contains(QT_ARCH, "arm64"): DEFINES += PCRE2_DISABLE_JIT
macos:contains(QT_ARCH, "arm64"): DEFINES += PCRE2_DISABLE_JIT

SOURCES += \
    $$PWD/src/pcre2_auto_possess.c \
    $$PWD/src/pcre2_chartables.c \
    $$PWD/src/pcre2_compile.c \
    $$PWD/src/pcre2_config.c \
    $$PWD/src/pcre2_context.c \
    $$PWD/src/pcre2_dfa_match.c \
    $$PWD/src/pcre2_error.c \
    $$PWD/src/pcre2_extuni.c \
    $$PWD/src/pcre2_find_bracket.c \
    $$PWD/src/pcre2_jit_compile.c \
    $$PWD/src/pcre2_maketables.c \
    $$PWD/src/pcre2_match.c \
    $$PWD/src/pcre2_match_data.c \
    $$PWD/src/pcre2_newline.c \
    $$PWD/src/pcre2_ord2utf.c \
    $$PWD/src/pcre2_pattern_info.c \
    $$PWD/src/pcre2_script_run.c \
    $$PWD/src/pcre2_serialize.c \
    $$PWD/src/pcre2_string_utils.c \
    $$PWD/src/pcre2_study.c \
    $$PWD/src/pcre2_substitute.c \
    $$PWD/src/pcre2_substring.c \
    $$PWD/src/pcre2_tables.c \
    $$PWD/src/pcre2_ucd.c \
    $$PWD/src/pcre2_valid_utf.c \
    $$PWD/src/pcre2_xclass.c

HEADERS += \
    $$PWD/src/config.h \
    $$PWD/src/pcre2.h \
    $$PWD/src/pcre2_internal.h \
    $$PWD/src/pcre2_intmodedep.h \
    $$PWD/src/pcre2_ucp.h
