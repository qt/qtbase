# Generated from compose.pro.

find_package(XKB)

pkg_get_variable(PKG_X11_PREFIX x11 prefix)

#####################################################################
## composeplatforminputcontextplugin Plugin:
#####################################################################

add_qt_plugin(composeplatforminputcontextplugin
    TYPE platforminputcontexts
    SOURCES
        generator/qtablegenerator.cpp generator/qtablegenerator.h
        qcomposeplatforminputcontext.cpp qcomposeplatforminputcontext.h
        qcomposeplatforminputcontextmain.cpp
    DEFINES
        X11_PREFIX="${PKG_X11_PREFIX}"
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        XKB::XKB
    # OTHER_FILES = "$$PWD/compose.json"
    # PLUGIN_CLASS_NAME = "QComposePlatformInputContextPlugin"
    # PLUGIN_EXTENDS = "-"
    # _LOADED = "qt_plugin"
)
