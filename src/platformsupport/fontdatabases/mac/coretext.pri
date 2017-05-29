HEADERS += $$PWD/qcoretextfontdatabase_p.h $$PWD/qfontengine_coretext_p.h
OBJECTIVE_SOURCES += $$PWD/qfontengine_coretext.mm $$PWD/qcoretextfontdatabase.mm

qtConfig(freetype) {
    QMAKE_USE_PRIVATE += freetype
    HEADERS += freetype/qfontengine_ft_p.h
    SOURCES += freetype/qfontengine_ft.cpp
}

LIBS_PRIVATE += \
    -framework CoreFoundation \
    -framework CoreGraphics \
    -framework CoreText \
    -framework Foundation

macos: \
    LIBS_PRIVATE += -framework AppKit
else: \
    LIBS_PRIVATE += -framework UIKit

CONFIG += watchos_coretext
