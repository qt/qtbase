INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD
QT += core-private

SOURCES += $$PWD/qsystemlock.cpp

HEADERS += $$PWD/qsystemlock.h \
           $$PWD/qsystemlock_p.h

unix:SOURCES += $$PWD/qsystemlock_unix.cpp
win32:SOURCES += $$PWD/qsystemlock_win.cpp
