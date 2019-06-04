# Generated from datatable.pro.

#####################################################################
## datatable Binary:
#####################################################################

add_qt_executable(datatable
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_datatable.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:datatable.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:datatable.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
