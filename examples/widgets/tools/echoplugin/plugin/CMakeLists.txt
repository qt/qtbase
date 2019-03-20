# Generated from plugin.pro.

#####################################################################
## qtLibraryTargetechoplugin Module:
#####################################################################

add_qt_plugin(echoplugin_plugin TYPE plugin # special case
    SOURCES
        echoplugin.cpp echoplugin.h
    INCLUDE_DIRECTORIES
        ../echowindow
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:plugin.pro:<NONE>:
# CONFIG = "plugin" "install_ok"
# DESTDIR = "../plugins"
# EXAMPLE_FILES = "echoplugin.json"
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_EXAMPLES]/widgets/tools/echoplugin/plugins"
