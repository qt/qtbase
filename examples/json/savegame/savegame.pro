QT += core
QT -= gui

TARGET = savegame
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

# install
target.path = $$[QT_INSTALL_EXAMPLES]/json/savegame
INSTALLS += target

SOURCES += main.cpp \
    character.cpp \
    game.cpp \
    level.cpp

HEADERS += \
    character.h \
    game.h \
    level.h
