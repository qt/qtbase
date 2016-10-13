darwin {
    include($$PWD/mac/coretext.pri)
} else {
    qtConfig(freetype) {
        include($$PWD/basic/basic.pri)
    }

    unix {
        CONFIG += qpa/genericunixfontdatabase
        include($$PWD/genericunix/genericunix.pri)
        qtConfig(fontconfig) {
            include($$PWD/fontconfig/fontconfig.pri)
        }
    }
}
