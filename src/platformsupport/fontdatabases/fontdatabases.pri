DEFINES += QT_COMPILES_IN_HARFBUZZ
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src
CONFIG += qpa/genericunixfontdatabase

!win32|contains(QT_CONFIG, freetype) {
    include($$PWD/basic/basic.pri)
}

unix {
    include($$PWD/genericunix/genericunix.pri)
    contains(QT_CONFIG,fontconfig) {
        include($$PWD/fontconfig/fontconfig.pri)
    }
}

