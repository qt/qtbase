qtConfig(xcb-native-painting) {
    qtConfig(xrender): QMAKE_USE += xrender
    qtConfig(fontconfig): QMAKE_USE_PRIVATE += freetype

    INCLUDEPATH += $$PWD
    HEADERS += \
        $$PWD/qtessellator_p.h \
        $$PWD/qpixmap_x11_p.h \
        $$PWD/qpaintengine_x11_p.h \
        $$PWD/qt_x11_p.h \
        $$PWD/qcolormap_x11_p.h \
        $$PWD/qbackingstore_x11_p.h \
        $$PWD/qxcbnativepainting.h

    SOURCES += \
        $$PWD/qtessellator.cpp \
        $$PWD/qpixmap_x11.cpp \
        $$PWD/qpaintengine_x11.cpp \
        $$PWD/qcolormap_x11.cpp \
        $$PWD/qbackingstore_x11.cpp \
        $$PWD/qxcbnativepainting.cpp
}
