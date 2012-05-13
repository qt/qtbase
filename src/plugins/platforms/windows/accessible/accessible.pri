SOURCES += \
    $$PWD/qwindowsmsaaaccessible.cpp \
    $$PWD/qwindowsaccessibility.cpp \
    $$PWD/comutils.cpp

HEADERS += \
    $$PWD/qwindowsmsaaaccessible.h \
    $$PWD/qwindowsaccessibility.h \
    $$PWD/comutils.h

!win32-g++*: {
    SOURCES += $$PWD/iaccessible2.cpp
    HEADERS += $$PWD/iaccessible2.h
    include(../../../../3rdparty/iaccessible2/iaccessible2.pri)
}

win32-g++*: LIBS *= -luuid