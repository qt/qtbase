TEMPLATE = app
TARGET = highdpi
INCLUDEPATH += .
QT += widgets gui-private
CONFIG +=console
CONFIG -= app_bundle
CONFIG += c++11
# Input
SOURCES += main.cpp

RESOURCES += \
    highdpi.qrc

