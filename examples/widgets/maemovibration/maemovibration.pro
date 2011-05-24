TARGET      = maemovibration
HEADERS     += buttonwidget.h mcevibrator.h
SOURCES     += main.cpp buttonwidget.cpp mcevibrator.cpp

# All generated files goes same directory
OBJECTS_DIR = build
MOC_DIR     = build
UI_DIR      = build
DESTDIR     = build

TEMPLATE    = app
CONFIG      += debug
QT=core gui

maemo5 {
    QT += dbus
    CONFIG += link_pkgconfig
    PKG_CONFIG += mce
    INSTALLS    += target
    target.path  = /usr/bin/

    INSTALLS    += desktop
    desktop.path  = /usr/share/applications/hildon
    desktop.files  = data/maemovibration.desktop
	
    INSTALLS    += service
    service.path  = /usr/share/dbus-1/services
    service.files  = data/maemovibration.service

    INSTALLS    += icon64
    icon64.path  = /usr/share/icons/hicolor/64x64/apps
    icon64.files  = data/64x64/maemovibration.png

    #
    # Targets for debian source and binary package creation
    #
    debian-src.commands = dpkg-buildpackage -S -r -us -uc -d
    debian-bin.commands = dpkg-buildpackage -b -r -uc -d
    debian-all.depends = debian-src debian-bin

    #
    # Clean all but Makefile
    #
    compiler_clean.commands = -$(DEL_FILE) $(TARGET)

    QMAKE_EXTRA_TARGETS += debian-all debian-src debian-bin compiler_clean
}

!maemo5 {
    error(The Maemo Vibration Example only works for the maemo target!)
}

