QT *= gui-private

SOURCES += \
    $$PWD/qwindowsfontdatabase.cpp \
    $$PWD/qwindowsfontdatabasebase.cpp \
    $$PWD/qwindowsfontengine.cpp \
    $$PWD/qwindowsnativeimage.cpp

HEADERS += \
    $$PWD/qwindowsfontdatabase_p.h \
    $$PWD/qwindowsfontdatabasebase_p.h \
    $$PWD/qwindowsfontengine_p.h \
    $$PWD/qwindowsnativeimage_p.h

qtConfig(freetype) {
    SOURCES += $$PWD/qwindowsfontdatabase_ft.cpp
    HEADERS += $$PWD/qwindowsfontdatabase_ft_p.h
    QMAKE_USE_PRIVATE += freetype
}

qtConfig(directwrite):qtConfig(direct2d) {
    qtConfig(directwrite3) {
        QMAKE_USE_PRIVATE += dwrite_3
        DEFINES *= QT_USE_DIRECTWRITE3 QT_USE_DIRECTWRITE2

        SOURCES += $$PWD/qwindowsdirectwritefontdatabase.cpp
        HEADERS += $$PWD/qwindowsdirectwritefontdatabase_p.h
    } else: qtConfig(directwrite2) {
        QMAKE_USE_PRIVATE += dwrite_2
        DEFINES *= QT_USE_DIRECTWRITE2
    } else {
        QMAKE_USE_PRIVATE += dwrite
    }
    QMAKE_USE_PRIVATE += d2d1

    SOURCES += $$PWD/qwindowsfontenginedirectwrite.cpp
    HEADERS += $$PWD/qwindowsfontenginedirectwrite_p.h
} else {
    DEFINES *= QT_NO_DIRECTWRITE
}

QMAKE_USE_PRIVATE += advapi32 ole32 user32 gdi32
mingw: QMAKE_USE_PRIVATE += uuid
