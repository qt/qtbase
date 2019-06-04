# Generated from blacklisted.pro.

#####################################################################
## blacklisted Binary:
#####################################################################

add_qt_executable(blacklisted
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_blacklisted.cpp
    LIBRARIES
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:blacklisted.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:blacklisted.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
