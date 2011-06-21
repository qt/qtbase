DEFINES += QT_COMPILES_IN_HARFBUZZ
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

unix {
    include($$PWD/basicunix/basicunix.pri)
    include($$PWD/genericunix/genericunix.pri)
    contains(QT_CONFIG,fontconfig) {
        include($$PWD/fontconfig/fontconfig.pri)
    }
}
