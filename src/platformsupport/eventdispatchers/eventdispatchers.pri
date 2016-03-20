unix {
SOURCES +=\
    $$PWD/qunixeventdispatcher.cpp\
    $$PWD/qgenericunixeventdispatcher.cpp\

HEADERS +=\
    $$PWD/qunixeventdispatcher_qpa_p.h\
    $$PWD/qgenericunixeventdispatcher_p.h\
} else: win32 {
SOURCES +=\
    $$PWD/qwindowsguieventdispatcher.cpp

HEADERS +=\
    $$PWD/qwindowsguieventdispatcher_p.h
}

contains(QT_CONFIG, glib) {
    SOURCES +=$$PWD/qeventdispatcher_glib.cpp
    HEADERS +=$$PWD/qeventdispatcher_glib_p.h
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_GLIB
    LIBS_PRIVATE += $$QMAKE_LIBS_GLIB
}
