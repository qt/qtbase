QT += network

QNETWORK_SRC = $$QT_SOURCE_TREE/src/network

INCLUDEPATH += $$QNETWORK_SRC

win32 {
    wince {
        LIBS += -lws2
    } else {
        LIBS += -lws2_32
    }
}

unix:contains(QT_CONFIG, reduce_exports) {
    SOURCES += $$QNETWORK_SRC/socket/qnativesocketengine_unix.cpp
    SOURCES += $$QNETWORK_SRC/socket/qnativesocketengine.cpp
    SOURCES += $$QNETWORK_SRC/socket/qabstractsocketengine.cpp
}
