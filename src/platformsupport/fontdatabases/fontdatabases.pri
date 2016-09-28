darwin:!if(watchos:CONFIG(simulator, simulator|device)) {
    include($$PWD/mac/coretext.pri)
} else {
    qtConfig(freetype) {
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
