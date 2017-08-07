# zlib dependency satisfied by bundled 3rd party zlib or system zlib
qtConfig(system-zlib) {
    QMAKE_USE_PRIVATE += zlib
} else {
    INCLUDEPATH +=  $$PWD/zlib/src
    !no_core_dep {
        CONFIG += qt
        QT_PRIVATE += core
    }
}
