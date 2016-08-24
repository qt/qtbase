darwin {
    include($$PWD/mac/coretext.pri)
} else {
    !win32|qtConfig(freetype) {
        include($$PWD/basic/basic.pri)
    }

    unix {
        CONFIG += qpa/genericunixfontdatabase
        include($$PWD/genericunix/genericunix.pri)
        contains(QT_CONFIG,fontconfig) {
            include($$PWD/fontconfig/fontconfig.pri)
        }
    }
}
