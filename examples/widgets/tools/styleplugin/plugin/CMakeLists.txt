# Generated from plugin.pro.

#####################################################################
## mplestyleplugin Module:
#####################################################################

add_qt_plugin(simplestyleplugin TYPE style # special case
    SOURCES
        simplestyle.cpp simplestyle.h
        simplestyleplugin.cpp simplestyleplugin.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:plugin.pro:<NONE>:
# CONFIG = "plugin" "install_ok"
# EXAMPLE_FILES = "simplestyle.json"
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_EXAMPLES]/widgets/tools/styleplugin/styles"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:plugin.pro:CONFIG(debug,release OR debug):
# DESTDIR = "../debug/styles/"

#### Keys ignored in scope 4:.:plugin.pro:CONFIG(release,release OR debug):
# DESTDIR = "../release/styles/"

#### Keys ignored in scope 5:.:plugin.pro:else:
# DESTDIR = "../styles/"
