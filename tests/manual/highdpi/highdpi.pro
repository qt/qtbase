TEMPLATE = app
TARGET = highdpi
INCLUDEPATH += .
QT += widgets gui-private
CONFIG += cmdline
CONFIG += c++11
# Input
SOURCES += \
        dragwidget.cpp \
        main.cpp

HEADERS += \
        dragwidget.h

RESOURCES += \
    highdpi.qrc

