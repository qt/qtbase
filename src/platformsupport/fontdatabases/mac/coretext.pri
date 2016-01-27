HEADERS += $$PWD/qcoretextfontdatabase_p.h $$PWD/qfontengine_coretext_p.h
OBJECTIVE_SOURCES += $$PWD/qfontengine_coretext.mm $$PWD/qcoretextfontdatabase.mm

contains(QT_CONFIG, freetype) {
    include($$QT_SOURCE_TREE/src/3rdparty/freetype_dependency.pri)
    HEADERS += $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h
    SOURCES += $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp
    CONFIG += opentype
}

ios: \
    # On iOS CoreText and CoreGraphics are stand-alone frameworks
    LIBS_PRIVATE += -framework CoreText -framework CoreGraphics
else: \
    # On Mac OS they are part of the ApplicationServices umbrella framework,
    # even in 10.8 where they were also made available stand-alone.
    LIBS_PRIVATE += -framework ApplicationServices
