QT *= testlib

SOURCES += \
        $$PWD/qbaselinetest.cpp

HEADERS += \
        $$PWD/qbaselinetest.h

qtHaveModule(widgets) {
        SOURCES += $$PWD/qwidgetbaselinetest.cpp
        HEADERS += $$PWD/qwidgetbaselinetest.h
}

include($$PWD/baselineprotocol.pri)
