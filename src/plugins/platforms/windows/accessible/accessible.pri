SOURCES += \
    $$PWD/qwindowsaccessibility.cpp

HEADERS += \
    $$PWD/qwindowsaccessibility.h

!*g++* {
    SOURCES += \
        $$PWD/qwindowsmsaaaccessible.cpp \
        $$PWD/iaccessible2.cpp \
        $$PWD/comutils.cpp

    HEADERS += \
        $$PWD/qwindowsmsaaaccessible.h \
        $$PWD/iaccessible2.h \
        $$PWD/comutils.h

    include(../../../../3rdparty/iaccessible2/iaccessible2.pri)
} # !g++
