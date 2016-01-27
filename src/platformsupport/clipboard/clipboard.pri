mac {
    HEADERS += $$PWD/qmacmime_p.h
    OBJECTIVE_SOURCES += $$PWD/qmacmime.mm

    osx: LIBS_PRIVATE += -framework AppKit
}

