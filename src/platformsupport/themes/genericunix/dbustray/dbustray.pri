QT_FOR_PRIVATE += dbus

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qdbustrayicon_p.h \
    $$PWD/qdbustraytypes_p.h \
    $$PWD/qstatusnotifieritemadaptor_p.h \
    $$PWD/qxdgnotificationproxy_p.h \

SOURCES += \
    $$PWD/qdbustrayicon.cpp \
    $$PWD/qdbustraytypes.cpp \
    $$PWD/qstatusnotifieritemadaptor.cpp \
    $$PWD/qxdgnotificationproxy.cpp \
