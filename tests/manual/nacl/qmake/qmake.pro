TEMPLATE = app
TARGET = hello_world
DEPENDPATH += .

# should be set by Qt, but isn't
INCLUDEPATH += $$(NACL_SDK_ROOT)/include

# Input
SOURCES += hello_world.cpp

LIBS += -lppapi
QT =