# Generated from failcleanup.pro.

#####################################################################
## failcleanup Binary:
#####################################################################

add_qt_executable(failcleanup
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_failcleanup.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:failcleanup.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:failcleanup.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
