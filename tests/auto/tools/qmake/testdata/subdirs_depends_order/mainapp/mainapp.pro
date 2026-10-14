TEMPLATE = app
CONFIG -= debug_and_release
QT -= gui
TARGET = mainapp
SOURCES = main.cpp
INCLUDEPATH += ../deplib
LIBS += -L../deplib -ldeplib
