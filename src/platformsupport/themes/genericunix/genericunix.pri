HEADERS += $$PWD/qgenericunixthemes_p.h
SOURCES += $$PWD/qgenericunixthemes.cpp

qtConfig(dbus) {
    include(dbusmenu/dbusmenu.pri)

    qtConfig(systemtrayicon) {
      include(dbustray/dbustray.pri)
    }
}
