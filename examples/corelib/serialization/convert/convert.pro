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
    datastreamconverter.cpp \
    debugtextdumper.cpp \
    jsonconverter.cpp \
    nullconverter.cpp \
    textconverter.cpp \
    xmlconverter.cpp

HEADERS += \
    converter.h \
    cborconverter.h \
    datastreamconverter.h \
    debugtextdumper.h \
    jsonconverter.h \
    nullconverter.h \
    textconverter.h \
    xmlconverter.h
