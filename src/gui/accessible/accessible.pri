# Qt accessibility module

contains(QT_CONFIG, accessibility) {
    HEADERS += \
        accessible/qaccessible.h \
        accessible/qaccessiblecache_p.h \
        accessible/qaccessible2_p.h \
        accessible/qaccessibleobject.h \
        accessible/qaccessibleplugin.h \
        accessible/qplatformaccessibility.h

    SOURCES += accessible/qaccessible.cpp \
        accessible/qaccessible2.cpp \
        accessible/qaccessiblecache.cpp \
        accessible/qaccessibleobject.cpp \
        accessible/qaccessibleplugin.cpp \
        accessible/qplatformaccessibility.cpp

    HEADERS += accessible/qaccessiblebridge.h
    SOURCES += accessible/qaccessiblebridge.cpp
}
