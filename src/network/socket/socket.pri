# Qt network socket

HEADERS += socket/qabstractsocketengine_p.h \
           socket/qabstractsocket.h \
           socket/qabstractsocket_p.h \
           socket/qtcpsocket.h \
           socket/qudpsocket.h \
           socket/qtcpserver.h \
           socket/qtcpsocket_p.h \
           socket/qtcpserver_p.h

SOURCES += socket/qabstractsocketengine.cpp \
           socket/qabstractsocket.cpp \
           socket/qtcpsocket.cpp \
           socket/qudpsocket.cpp \
           socket/qtcpserver.cpp

# SOCK5 support.

qtConfig(socks5) {
    HEADERS += \
        socket/qsocks5socketengine_p.h
    SOURCES += \
        socket/qsocks5socketengine.cpp
}

qtConfig(http) {
    HEADERS += \
        socket/qhttpsocketengine_p.h
    SOURCES += \
        socket/qhttpsocketengine.cpp
}

# SCTP support.

qtConfig(sctp) {
    HEADERS += socket/qsctpserver.h \
                socket/qsctpserver_p.h \
                socket/qsctpsocket.h \
                socket/qsctpsocket_p.h

    SOURCES += socket/qsctpserver.cpp \
                socket/qsctpsocket.cpp
}

!winrt {
    SOURCES += socket/qnativesocketengine.cpp
    HEADERS += socket/qnativesocketengine_p.h
}

unix {
    SOURCES += socket/qnativesocketengine_unix.cpp
    HEADERS += socket/qnet_unix_p.h
}

# Suppress deprecation warnings with moc because MS headers have
# invalid C/C++ code otherwise.
msvc: QMAKE_MOC_OPTIONS += -D_WINSOCK_DEPRECATED_NO_WARNINGS

win32:!winrt:SOURCES += socket/qnativesocketengine_win.cpp
win32:!winrt:LIBS_PRIVATE += -ladvapi32

winrt {
    SOURCES += socket/qnativesocketengine_winrt.cpp
    HEADERS += socket/qnativesocketengine_winrt_p.h
}

qtConfig(localserver) {
    HEADERS += socket/qlocalserver.h \
               socket/qlocalserver_p.h \
               socket/qlocalsocket.h \
               socket/qlocalsocket_p.h
    SOURCES += socket/qlocalsocket.cpp \
               socket/qlocalserver.cpp

    integrity|winrt {
        SOURCES += socket/qlocalsocket_tcp.cpp \
                   socket/qlocalserver_tcp.cpp
        DEFINES += QT_LOCALSOCKET_TCP
    } else: unix {
        SOURCES += socket/qlocalsocket_unix.cpp \
                   socket/qlocalserver_unix.cpp
    } else: win32 {
        SOURCES += socket/qlocalsocket_win.cpp \
                   socket/qlocalserver_win.cpp
    }
}

qtConfig(system-proxies) {
    DEFINES += QT_USE_SYSTEM_PROXIES
}
