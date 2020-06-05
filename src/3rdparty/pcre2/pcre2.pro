TARGET = qtpcre2

CONFIG += \
    static \
    hide_symbols \
    exceptions_off rtti_off warn_off

include(pcre2.pri)

# platform/compiler specific definitions
uikit|qnx: DEFINES += PCRE2_DISABLE_JIT
win32:contains(QT_ARCH, "arm"): DEFINES += PCRE2_DISABLE_JIT
win32:contains(QT_ARCH, "arm64"): DEFINES += PCRE2_DISABLE_JIT

load(qt_helper_lib)
