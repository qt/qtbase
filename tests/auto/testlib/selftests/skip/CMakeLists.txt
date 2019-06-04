# Generated from skip.pro.

#####################################################################
## skip Binary:
#####################################################################

add_qt_executable(skip
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_skip.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:skip.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:skip.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
