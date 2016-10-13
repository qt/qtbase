QT_FOR_PRIVATE += dbus

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/qdbusmenuadaptor_p.h \
    $$PWD/qdbusmenutypes_p.h \
    $$PWD/qdbusmenuconnection_p.h \
    $$PWD/qdbusmenubar_p.h \
    $$PWD/qdbusmenuregistrarproxy_p.h \
    $$PWD/qdbusplatformmenu_p.h \

SOURCES += \
    $$PWD/qdbusmenuadaptor.cpp \
    $$PWD/qdbusmenutypes.cpp \
    $$PWD/qdbusmenuconnection.cpp \
    $$PWD/qdbusmenubar.cpp \
    $$PWD/qdbusmenuregistrarproxy.cpp \
    $$PWD/qdbusplatformmenu.cpp \
