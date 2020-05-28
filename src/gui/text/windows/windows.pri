SOURCES += \
    text/windows/qwindowsfontdatabase.cpp \
    text/windows/qwindowsfontdatabasebase.cpp \
    text/windows/qwindowsfontengine.cpp \
    text/windows/qwindowsnativeimage.cpp

HEADERS += \
    text/windows/qwindowsfontdatabase_p.h \
    text/windows/qwindowsfontdatabasebase_p.h \
    text/windows/qwindowsfontengine_p.h \
    text/windows/qwindowsnativeimage_p.h

qtConfig(freetype) {
    SOURCES += text/windows/qwindowsfontdatabase_ft.cpp
    HEADERS += text/windows/qwindowsfontdatabase_ft_p.h
    QMAKE_USE_PRIVATE += freetype
}

qtConfig(directwrite):qtConfig(direct2d) {
    qtConfig(directwrite3) {
        QMAKE_USE_PRIVATE += dwrite_3
        DEFINES *= QT_USE_DIRECTWRITE3 QT_USE_DIRECTWRITE2

        SOURCES += text/windows/qwindowsdirectwritefontdatabase.cpp
        HEADERS += text/windows/qwindowsdirectwritefontdatabase_p.h
    } else: qtConfig(directwrite2) {
        QMAKE_USE_PRIVATE += dwrite_2
        DEFINES *= QT_USE_DIRECTWRITE2
    } else {
        QMAKE_USE_PRIVATE += dwrite
    }
    QMAKE_USE_PRIVATE += d2d1

    SOURCES += text/windows/qwindowsfontenginedirectwrite.cpp
    HEADERS += text/windows/qwindowsfontenginedirectwrite_p.h
} else {
    DEFINES *= QT_NO_DIRECTWRITE
}

QMAKE_USE_PRIVATE += advapi32 ole32 user32 gdi32
mingw: QMAKE_USE_PRIVATE += uuid
