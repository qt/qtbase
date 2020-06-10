SOURCES += \
    platform/unix/qunixeventdispatcher.cpp \
    platform/unix/qgenericunixeventdispatcher.cpp

HEADERS += \
    platform/unix/qunixeventdispatcher_qpa_p.h \
    platform/unix/qgenericunixeventdispatcher_p.h

qtConfig(glib) {
    SOURCES += platform/unix/qeventdispatcher_glib.cpp
    HEADERS += platform/unix/qeventdispatcher_glib_p.h
    QMAKE_USE_PRIVATE += glib
}
