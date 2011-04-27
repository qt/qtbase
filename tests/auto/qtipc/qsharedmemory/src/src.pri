INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

SOURCES += $$PWD/qsystemlock.cpp

HEADERS += $$PWD/qsystemlock.h \
           $$PWD/qsystemlock_p.h

unix:SOURCES += $$PWD/qsystemlock_unix.cpp
win32:SOURCES += $$PWD/qsystemlock_win.cpp
