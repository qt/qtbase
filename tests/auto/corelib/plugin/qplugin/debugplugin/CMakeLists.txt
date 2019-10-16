# Generated from debugplugin.pro.

#####################################################################
## debugplugin Plugin:
#####################################################################

add_qt_plugin(debugplugin
    SKIP_INSTALL
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../plugins"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:debugplugin.pro:<TRUE>:
# TEMPLATE = "lib"
