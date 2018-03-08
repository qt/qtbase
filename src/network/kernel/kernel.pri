# Qt network kernel module

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
INCLUDEPATH += $$PWD

HEADERS += kernel/qtnetworkglobal.h \
           kernel/qtnetworkglobal_p.h \
           kernel/qauthenticator.h \
           kernel/qauthenticator_p.h \
           kernel/qhostaddress.h \
           kernel/qhostaddress_p.h \
           kernel/qhostinfo.h \
           kernel/qhostinfo_p.h \
           kernel/qnetworkdatagram.h \
           kernel/qnetworkdatagram_p.h \
           kernel/qnetworkinterface.h \
           kernel/qnetworkinterface_p.h \
           kernel/qnetworkinterface_unix_p.h \
           kernel/qnetworkproxy.h

SOURCES += kernel/qauthenticator.cpp \
           kernel/qhostaddress.cpp \
           kernel/qhostinfo.cpp \
           kernel/qnetworkdatagram.cpp \
           kernel/qnetworkinterface.cpp \
           kernel/qnetworkproxy.cpp

qtConfig(ftp) {
    HEADERS += kernel/qurlinfo_p.h
    SOURCES += kernel/qurlinfo.cpp
}

qtConfig(dnslookup) {
    HEADERS += kernel/qdnslookup.h \
               kernel/qdnslookup_p.h

    SOURCES += kernel/qdnslookup.cpp
}

unix {
    !integrity:qtConfig(dnslookup): SOURCES += kernel/qdnslookup_unix.cpp
    SOURCES += kernel/qhostinfo_unix.cpp

    qtConfig(linux-netlink): SOURCES += kernel/qnetworkinterface_linux.cpp
    else: SOURCES += kernel/qnetworkinterface_unix.cpp
}

android:qtConfig(dnslookup) {
    SOURCES -= kernel/qdnslookup_unix.cpp
    SOURCES += kernel/qdnslookup_android.cpp
}

win32: {
    SOURCES += kernel/qhostinfo_win.cpp

    !winrt {
        SOURCES += kernel/qnetworkinterface_win.cpp
        qtConfig(dnslookup): SOURCES += kernel/qdnslookup_win.cpp
        LIBS_PRIVATE += -ldnsapi -liphlpapi
    } else {
        SOURCES += kernel/qnetworkinterface_winrt.cpp
        qtConfig(dnslookup): SOURCES += kernel/qdnslookup_winrt.cpp
    }
}

mac {
    LIBS_PRIVATE += -framework CoreFoundation
    !uikit: LIBS_PRIVATE += -framework CoreServices -framework SystemConfiguration
}

uikit:HEADERS += kernel/qnetworkinterface_uikit_p.h
osx:SOURCES += kernel/qnetworkproxy_mac.cpp
else:win32:!winrt: SOURCES += kernel/qnetworkproxy_win.cpp
else: qtConfig(libproxy) {
    SOURCES += kernel/qnetworkproxy_libproxy.cpp
    QMAKE_USE_PRIVATE += libproxy libdl
}
else:SOURCES += kernel/qnetworkproxy_generic.cpp
