SOURCES += findtestdata.cpp
QT = core testlib

mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

RESOURCES = findtestdata.qrc

TARGET = findtestdata
