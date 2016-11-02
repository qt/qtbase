QT *= gui-private

SOURCES += \
    $$PWD/qwindowsfontdatabase.cpp \
    $$PWD/qwindowsfontengine.cpp \
    $$PWD/qwindowsnativeimage.cpp

HEADERS += \
    $$PWD/qwindowsfontdatabase_p.h \
    $$PWD/qwindowsfontengine_p.h \
    $$PWD/qwindowsnativeimage_p.h

qtConfig(freetype) {
    SOURCES += $$PWD/qwindowsfontdatabase_ft.cpp
    HEADERS += $$PWD/qwindowsfontdatabase_ft_p.h
    qtConfig(system-freetype) {
        include($$QT_SOURCE_TREE/src/platformsupport/fontdatabases/basic/basic.pri)
    } else {
        include($$QT_SOURCE_TREE/src/3rdparty/freetype_dependency.pri)
    }
}

qtConfig(directwrite) {
    qtConfig(directwrite2): \
        DEFINES *= QT_USE_DIRECTWRITE2

    SOURCES += $$PWD/qwindowsfontenginedirectwrite.cpp
    HEADERS += $$PWD/qwindowsfontenginedirectwrite_p.h
} else {
    DEFINES *= QT_NO_DIRECTWRITE
}

LIBS += -lole32 -lgdi32 -luser32
