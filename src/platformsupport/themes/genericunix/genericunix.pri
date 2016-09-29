HEADERS += $$PWD/qgenericunixthemes_p.h
SOURCES += $$PWD/qgenericunixthemes.cpp

qtConfig(dbus) {
    include(dbusmenu/dbusmenu.pri)
    include(dbustray/dbustray.pri)
}
