# Qt core library plugin module

HEADERS += \
    plugin/qfactoryinterface.h \
    plugin/qpluginloader.h \
    plugin/qplugin.h \
    plugin/quuid.h \
    plugin/qfactoryloader_p.h

SOURCES += \
    plugin/qfactoryinterface.cpp \
    plugin/qpluginloader.cpp \
    plugin/qfactoryloader.cpp \
    plugin/quuid.cpp

win32 {
    HEADERS += plugin/qsystemlibrary_p.h
    SOURCES += plugin/qsystemlibrary.cpp
}

qtConfig(library) {
    HEADERS += \
        plugin/qlibrary.h \
        plugin/qlibrary_p.h \
        plugin/qelfparser_p.h \
        plugin/qmachparser_p.h

    SOURCES += \
        plugin/qlibrary.cpp \
        plugin/qelfparser_p.cpp \
        plugin/qmachparser.cpp

    unix: SOURCES += plugin/qlibrary_unix.cpp
    else: SOURCES += plugin/qlibrary_win.cpp

    qtConfig(dlopen): QMAKE_USE_PRIVATE += libdl
}
