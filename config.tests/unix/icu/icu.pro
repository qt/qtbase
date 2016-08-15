SOURCES = icu.cpp
CONFIG += console
CONFIG -= qt dylib

CONFIG += build_all
CONFIG(debug, debug|release): \
    LIBS += $$LIBS_DEBUG
else: \
    LIBS += $$LIBS_RELEASE
