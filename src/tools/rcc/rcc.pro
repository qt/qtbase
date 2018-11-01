option(host_build)
CONFIG += force_bootstrap

DEFINES += QT_RCC QT_NO_CAST_FROM_ASCII QT_NO_FOREACH

include(rcc.pri)
SOURCES += main.cpp

QMAKE_TARGET_DESCRIPTION = "Qt Resource Compiler"
load(qt_tool)

# RCC is a bootstrapped tool, so qglobal.h #includes qconfig-bootstrapped.h
# and that has a #define saying zstd isn't present (for qresource.cpp, which is
# part of the bootstrap lib). So we inform the presence of the feature in the
# command-line.
qtConfig(zstd):!cross_compile {
    DEFINES += QT_FEATURE_zstd=1
    QMAKE_USE_PRIVATE += zstd
} else {
    DEFINES += QT_FEATURE_zstd=-1
}
