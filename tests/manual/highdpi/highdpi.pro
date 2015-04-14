TEMPLATE = app
TARGET = highdpi
INCLUDEPATH += .
QT += widgets gui-private
CONFIG+=console
CONFIG -= app_bundle
# Input
SOURCES += main.cpp

RESOURCES += \
    highdpi.qrc

