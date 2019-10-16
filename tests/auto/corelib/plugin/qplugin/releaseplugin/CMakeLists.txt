# Generated from releaseplugin.pro.

#####################################################################
## releaseplugin Plugin:
#####################################################################

add_qt_plugin(releaseplugin
    SKIP_INSTALL
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../plugins"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)

#### Keys ignored in scope 1:.:.:releaseplugin.pro:<TRUE>:
# TEMPLATE = "lib"
