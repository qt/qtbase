# Qt styles module

HEADERS += \
        styles/qdrawutil.h \
        styles/qstyle.h \
        styles/qstyleanimation_p.h \
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
        styles/qstylesheetstyle_p.h

SOURCES += \
        styles/qdrawutil.cpp \
        styles/qstyle.cpp \
        styles/qstyleanimation.cpp \
        styles/qstylefactory.cpp \
        styles/qstyleoption.cpp \
        styles/qstyleplugin.cpp \
        styles/qstylehelper.cpp \
        styles/qcommonstyle.cpp \
        styles/qproxystyle.cpp \
        styles/qstylepainter.cpp \
        styles/qstylesheetstyle.cpp \
        styles/qstylesheetstyle_default.cpp

wince* {
    RESOURCES += styles/qstyle_wince.qrc
} else {
    RESOURCES += styles/qstyle.qrc
}

contains( styles, all ) {
    styles = fusion mac windows windowsxp windowsvista
}

!macx-*|ios:styles -= mac

contains(QT_CONFIG, gtkstyle) {
    QMAKE_CXXFLAGS += $$QT_CFLAGS_QGTKSTYLE
    LIBS_PRIVATE += $$QT_LIBS_QGTKSTYLE
    styles += gtk
    CONFIG += x11
}

contains( styles, mac ) {
    HEADERS += \
        styles/qmacstyle_mac.h \
        styles/qmacstylepixmaps_mac_p.h \
        styles/qmacstyle_mac_p.h
        OBJECTIVE_SOURCES += styles/qmacstyle_mac.mm

    !contains( styles, windows ) {
        message( mac requires windows )
        styles += windows
        DEFINES+= QT_STYLE_WINDOWS
    }
} else {
    DEFINES += QT_NO_STYLE_MAC
}

contains( styles, windows ) {
    HEADERS += styles/qwindowsstyle.h
    SOURCES += styles/qwindowsstyle.cpp
    DEFINES += QT_STYLE_WINDOWS
} else {
    DEFINES += QT_NO_STYLE_WINDOWS
}

contains( styles, windowsvista ) {
    HEADERS += styles/qwindowsvistastyle.h
    HEADERS += styles/qwindowsvistastyle_p.h
    SOURCES += styles/qwindowsvistastyle.cpp
    !contains( styles, windowsxp ) {
        message( windowsvista requires windowsxp )
        styles += windowsxp
        DEFINES += QT_STYLE_WINDOWSXP
    }
} else {
    DEFINES += QT_NO_STYLE_WINDOWSVISTA
}

contains( styles, windowsxp ) {
    HEADERS += styles/qwindowsxpstyle.h
    SOURCES += styles/qwindowsxpstyle.cpp
    !contains( styles, windows ) {
        message( windowsxp requires windows )
        styles  += windows
        DEFINES += QT_STYLE_WINDOWS
    }
} else {
    DEFINES += QT_NO_STYLE_WINDOWSXP
}

contains( styles, gtk ) {
        HEADERS += styles/qgtkstyle.h
        HEADERS += styles/qgtkpainter_p.h
        HEADERS += styles/qgtkstyle_p.h
        SOURCES += styles/qgtkstyle.cpp
        SOURCES += styles/qgtkpainter.cpp
        SOURCES += styles/qgtkstyle_p.cpp
} else {
    DEFINES += QT_NO_STYLE_GTK
}
contains( styles, fusion ) {
        HEADERS += styles/qfusionstyle.h
        HEADERS += styles/qfusionstyle_p.h
        SOURCES += styles/qfusionstyle.cpp
} else {
    DEFINES += QT_NO_STYLE_FUSION
}

contains( styles, windowsce ) {
    HEADERS += styles/qwindowscestyle.h
    SOURCES += styles/qwindowscestyle.cpp
} else {
    DEFINES += QT_NO_STYLE_WINDOWSCE
}

contains( styles, windowsmobile ) {
    HEADERS += styles/qwindowsmobilestyle.h
    SOURCES += styles/qwindowsmobilestyle.cpp
} else {
    DEFINES += QT_NO_STYLE_WINDOWSMOBILE
}
