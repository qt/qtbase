DEFINES += PCRE_HAVE_CONFIG_H

# man 3 pcrejit for a list of supported platforms;
# as PCRE 8.30, stable JIT support is available for:
# - ARM v5, v7, and Thumb2
# - x86/x86-64
# - MIPS 32bit
equals(QT_ARCH, "i386")|equals(QT_ARCH, "x86_64")|equals(QT_ARCH, "arm")|if(equals(QT_ARCH, "mips"):!*-64) {
    DEFINES += SUPPORT_JIT
}

win32:DEFINES += PCRE_STATIC

INCLUDEPATH += $$PWD/pcre
SOURCES += \
    $$PWD/pcre/pcre16_byte_order.c \
    $$PWD/pcre/pcre16_chartables.c \
    $$PWD/pcre/pcre16_compile.c \
    $$PWD/pcre/pcre16_config.c \
    $$PWD/pcre/pcre16_dfa_exec.c \
    $$PWD/pcre/pcre16_exec.c \
    $$PWD/pcre/pcre16_fullinfo.c \
    $$PWD/pcre/pcre16_get.c \
    $$PWD/pcre/pcre16_globals.c \
    $$PWD/pcre/pcre16_jit_compile.c \
    $$PWD/pcre/pcre16_maketables.c \
    $$PWD/pcre/pcre16_newline.c \
    $$PWD/pcre/pcre16_ord2utf16.c \
    $$PWD/pcre/pcre16_refcount.c \
    $$PWD/pcre/pcre16_string_utils.c \
    $$PWD/pcre/pcre16_study.c \
    $$PWD/pcre/pcre16_tables.c \
    $$PWD/pcre/pcre16_ucd.c \
    $$PWD/pcre/pcre16_utf16_utils.c \
    $$PWD/pcre/pcre16_valid_utf16.c \
    $$PWD/pcre/pcre16_version.c \
    $$PWD/pcre/pcre16_xclass.c

