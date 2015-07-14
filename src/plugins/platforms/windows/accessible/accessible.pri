SOURCES += \
    $$PWD/qwindowsaccessibility.cpp \
    $$PWD/comutils.cpp

HEADERS += \
    $$PWD/qwindowsaccessibility.h \
    $$PWD/comutils.h

!wince: {
    SOURCES += $$PWD/qwindowsmsaaaccessible.cpp
    HEADERS += $$PWD/qwindowsmsaaaccessible.h

    !mingw: {
        SOURCES += $$PWD/iaccessible2.cpp
        HEADERS += $$PWD/iaccessible2.h
        include(../../../../3rdparty/iaccessible2/iaccessible2.pri)
    }
}

mingw: LIBS *= -luuid
