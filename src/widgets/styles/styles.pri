# Qt styles module

HEADERS += \
        styles/qdrawutil.h \
        styles/qstyle.h \
        styles/qstylefactory.h \
        styles/qstyleoption.h \
        styles/qstyleplugin.h \
        styles/qcommonstylepixmaps_p.h \
        styles/qcommonstyle.h \
        styles/qstylehelper_p.h \
        styles/qproxystyle.h \
        styles/qproxystyle_p.h \
        styles/qcommonstyle_p.h \
        styles/qstylepainter.h \
        styles/qstylesheetstyle_p.h \
        styles/qpixmapstyle_p.h \
        styles/qpixmapstyle_p_p.h

SOURCES += \
        styles/qdrawutil.cpp \
        styles/qstyle.cpp \
        styles/qstylefactory.cpp \
        styles/qstyleoption.cpp \
        styles/qstyleplugin.cpp \
        styles/qstylehelper.cpp \
        styles/qcommonstyle.cpp \
        styles/qproxystyle.cpp \
        styles/qstylepainter.cpp \
        styles/qstylesheetstyle.cpp \
        styles/qstylesheetstyle_default.cpp \
        styles/qpixmapstyle.cpp

RESOURCES += styles/qstyle.qrc

qtConfig(animation) {
    HEADERS += styles/qstyleanimation_p.h
    SOURCES += styles/qstyleanimation.cpp
}

qtConfig(style-windows) {
    HEADERS += styles/qwindowsstyle_p.h styles/qwindowsstyle_p_p.h
    SOURCES += styles/qwindowsstyle.cpp
}

qtConfig(style-fusion) {
    HEADERS += styles/qfusionstyle_p.h styles/qfusionstyle_p_p.h
    SOURCES += styles/qfusionstyle.cpp
}
