DEFINES += QT_COMPILES_IN_HARFBUZZ
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

!win32|contains(QT_CONFIG, freetype):!mac {
    include($$PWD/basic/basic.pri)
}

unix:!mac {
    CONFIG += qpa/genericunixfontdatabase
    include($$PWD/genericunix/genericunix.pri)
    contains(QT_CONFIG,fontconfig) {
        include($$PWD/fontconfig/fontconfig.pri)
    }
}

mac {
    include($$PWD/mac/coretext.pri)
}

