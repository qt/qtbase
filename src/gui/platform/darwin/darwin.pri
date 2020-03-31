HEADERS += $$PWD/qmacmime_p.h
SOURCES += $$PWD/qmacmime.mm
LIBS += -framework ImageIO
macos: LIBS_PRIVATE += -framework AppKit
