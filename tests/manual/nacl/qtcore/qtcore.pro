TEMPLATE = app
TARGET = hello_world_qtcore
DEPENDPATH += .

# should be set by Qt, but isn't
INCLUDEPATH += $$(NACL_SDK_ROOT)/include
               .
# Input
SOURCES += hello_world.cpp

LIBS +=  -lppapi -lnacl_io
QT = core