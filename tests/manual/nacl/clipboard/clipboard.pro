TEMPLATE = app
TARGET = main
DEPENDPATH += .

# should be set by Qt, but isn't
INCLUDEPATH += $$(NACL_SDK_ROOT)/include

# Input
SOURCES += main.cpp

LIBS += -lppapi -lppapi_cpp 
QT =