QT *= testlib

SOURCES += \
        $$PWD/qbaselinetest.cpp

HEADERS += \
        $$PWD/qbaselinetest.h

win32:MKSPEC=$$replace(QMAKESPEC, \\\\, /)
else:MKSPEC=$$QMAKESPEC
DEFINES += QMAKESPEC=\\\"$$MKSPEC\\\"

include($$PWD/baselineprotocol.pri)
