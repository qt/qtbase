contains(QT_CONFIG, dbus):contains(QT_CONFIG, xcb):contains(QT_CONFIG, accessibility) {

    QT += dbus
    include(../../3rdparty/atspi2/atspi2.pri)

    INCLUDEPATH += $$PWD

    HEADERS += \
        $$PWD/application_p.h \
        $$PWD/bridge_p.h \
        $$PWD/cache_p.h  \
        $$PWD/struct_marshallers_p.h \
        $$PWD/constant_mappings_p.h \
        $$PWD/dbusconnection_p.h \
        $$PWD/atspiadaptor_p.h

    SOURCES += \
        $$PWD/application.cpp \
        $$PWD/bridge.cpp \
        $$PWD/cache.cpp  \
        $$PWD/struct_marshallers.cpp \
        $$PWD/constant_mappings.cpp \
        $$PWD/dbusconnection.cpp \
        $$PWD/atspiadaptor.cpp
}
