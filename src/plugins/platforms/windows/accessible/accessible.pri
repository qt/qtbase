SOURCES += \
    $$PWD/qwindowsaccessibility.cpp \
    $$PWD/comutils.cpp

HEADERS += \
    $$PWD/qwindowsaccessibility.h \
    $$PWD/comutils.h

SOURCES += \
    $$PWD/qwindowsmsaaaccessible.cpp \
    $$PWD/iaccessible2.cpp

HEADERS += \
    $$PWD/qwindowsmsaaaccessible.h \
    $$PWD/iaccessible2.h

include(../../../../3rdparty/iaccessible2/iaccessible2.pri)

mingw: LIBS *= -luuid
