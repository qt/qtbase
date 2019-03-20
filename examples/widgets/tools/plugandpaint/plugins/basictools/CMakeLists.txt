# Generated from basictools.pro.

#####################################################################
## qtLibraryTargetpnp_basictools Module:
#####################################################################

add_qt_plugin(pnp_basictools # special case
    STATIC # special case
    OUTPUT_DIRECTORY ../../plugins # special case
    INSTALL_DIRECTORY ../../plugins # special case
    SOURCES
        basictoolsplugin.cpp basictoolsplugin.h
    INCLUDE_DIRECTORIES
        ../../app
    LIBRARIES
        Qt::Widgets
)

# CONFIG = "plugin" "static" "install_ok"
# DESTDIR = "../../plugins"
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_EXAMPLES]/widgets/tools/plugandpaint/plugins"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:basictools.pro:APPLE_UIKIT:
# CONFIG = "debug_and_release"
