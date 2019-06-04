# Generated from maxwarnings.pro.

#####################################################################
## maxwarnings Binary:
#####################################################################

add_qt_executable(maxwarnings
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        maxwarnings.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:maxwarnings.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:maxwarnings.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
