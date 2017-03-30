HEADERS += $$PWD/qcoretextfontdatabase_p.h $$PWD/qfontengine_coretext_p.h
OBJECTIVE_SOURCES += $$PWD/qfontengine_coretext.mm $$PWD/qcoretextfontdatabase.mm

qtConfig(freetype) {
    QMAKE_USE_PRIVATE += freetype
    HEADERS += basic/qfontengine_ft_p.h
    SOURCES += basic/qfontengine_ft.cpp
}

uikit: \
    # On iOS/tvOS/watchOS CoreText and CoreGraphics are stand-alone frameworks
    LIBS_PRIVATE += -framework CoreText -framework CoreGraphics -framework UIKit
else: \
    # On macOS they are re-exported by the AppKit framework
    LIBS_PRIVATE += -framework AppKit

CONFIG += watchos_coretext
