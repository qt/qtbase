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
}

qtConfig(directwrite) {
    qtConfig(directwrite2): \
        DEFINES *= QT_USE_DIRECTWRITE2

    SOURCES += $$PWD/qwindowsfontenginedirectwrite.cpp
    HEADERS += $$PWD/qwindowsfontenginedirectwrite_p.h
} else {
    DEFINES *= QT_NO_DIRECTWRITE
}

LIBS += -lole32 -lgdi32 -luser32 -ladvapi32
mingw: LIBS += -luuid
