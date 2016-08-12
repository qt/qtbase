# zlib dependency satisfied by bundled 3rd party zlib or system zlib
contains(QT_CONFIG, system-zlib) {
    QMAKE_USE_PRIVATE += zlib
} else {
    INCLUDEPATH +=  $$PWD/zlib
    !no_core_dep {
        CONFIG += qt
        QT_PRIVATE += core
    }
}
