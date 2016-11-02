HEADERS += $$PWD/qcoretextfontdatabase_p.h $$PWD/qfontengine_coretext_p.h
OBJECTIVE_SOURCES += $$PWD/qfontengine_coretext.mm $$PWD/qcoretextfontdatabase.mm

qtConfig(freetype) {
    QMAKE_USE_PRIVATE += freetype
    HEADERS += basic/qfontengine_ft_p.h
    SOURCES += basic/qfontengine_ft.cpp
}

uikit: \
    # On iOS/tvOS/watchOS CoreText and CoreGraphics are stand-alone frameworks
    LIBS_PRIVATE += -framework CoreText -framework CoreGraphics
else: \
    # On Mac OS they are part of the ApplicationServices umbrella framework,
    # even in 10.8 where they were also made available stand-alone.
    LIBS_PRIVATE += -framework ApplicationServices
