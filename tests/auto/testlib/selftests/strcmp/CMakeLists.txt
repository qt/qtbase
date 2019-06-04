# Generated from strcmp.pro.

#####################################################################
## strcmp Binary:
#####################################################################

add_qt_executable(strcmp
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_strcmp.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:strcmp.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:strcmp.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
