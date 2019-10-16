# Generated from staticplugin.pro.

#####################################################################
## staticplugin Plugin:
#####################################################################

add_qt_plugin(staticplugin
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SKIP_INSTALL
    STATIC
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Core
    MOC_OPTIONS
        "-M"
        "ExtraMetaData=StaticPlugin"
        "-M"
        "ExtraMetaData=foo"
)

#### Keys ignored in scope 1:.:.:staticplugin.pro:<TRUE>:
# TEMPLATE = "lib"
