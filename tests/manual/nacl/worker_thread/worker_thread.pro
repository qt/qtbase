TEMPLATE = app
TARGET = main
DEPENDPATH += .

# should be set by Qt, but isn't
INCLUDEPATH += $$(NACL_SDK_ROOT)/include

# Input
SOURCES += \
    main.cpp
    
QT = core gui
