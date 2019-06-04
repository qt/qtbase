# Generated from subtest.pro.

#####################################################################
## subtest Binary:
#####################################################################

add_qt_executable(subtest
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_subtest.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:subtest.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:subtest.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
