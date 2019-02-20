QT += core
QT -= gui

TARGET = convert
CONFIG += cmdline

TEMPLATE = app

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/serialization/convert
INSTALLS += target

SOURCES += main.cpp \
    cborconverter.cpp \
    jsonconverter.cpp \
    datastreamconverter.cpp \
    textconverter.cpp \
    xmlconverter.cpp \
    nullconverter.cpp

HEADERS += \
    converter.h \
    cborconverter.h \
    jsonconverter.h \
    datastreamconverter.h \
    textconverter.h \
    xmlconverter.h \
    nullconverter.h
