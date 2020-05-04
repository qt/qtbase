QT = core testlib
mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

INCLUDEPATH += ../../../../shared/
HEADERS = ../../../../shared/emulationdetector.h
SOURCES += tst_float.cpp
TARGET = float

include($$QT_SOURCE_TREE/src/testlib/selfcover.pri)
