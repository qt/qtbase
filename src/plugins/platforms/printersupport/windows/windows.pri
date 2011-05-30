INCLUDEPATH += $$PWD $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

HEADERS += $$PWD/qwindowsprintersupport.h \
    $$PWD/qprintengine_win_p.h
SOURCES += $$PWD/qwindowsprintersupport.cpp \
    $$PWD/qprintengine_win.cpp
QT += core-private widgets-private
