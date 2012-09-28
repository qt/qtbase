SOURCES = fontconfig.cpp
CONFIG -= qt app_bundle
LIBS += -lfreetype -lfontconfig
include(../../unix/freetype/freetype.pri)
