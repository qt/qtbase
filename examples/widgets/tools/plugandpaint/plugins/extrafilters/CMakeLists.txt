# Generated from extrafilters.pro.

#####################################################################
## qtLibraryTargetpnp_extrafilters Module:
#####################################################################

add_qt_plugin(pnp_extrafilters_plugin TYPE plugin # special case
    SOURCES
        extrafiltersplugin.cpp extrafiltersplugin.h
    INCLUDE_DIRECTORIES
        ../../app
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:extrafilters.pro:<NONE>:
# CONFIG = "plugin" "install_ok"
# DESTDIR = "../../plugins"
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_EXAMPLES]/widgets/tools/plugandpaint/plugins"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:extrafilters.pro:APPLE_UIKIT:
# CONFIG = "debug_and_release"
