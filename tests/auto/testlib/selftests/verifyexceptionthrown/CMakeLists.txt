# Generated from verifyexceptionthrown.pro.

#####################################################################
## verifyexceptionthrown Binary:
#####################################################################

add_qt_executable(verifyexceptionthrown
    EXCEPTIONS
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_verifyexceptionthrown.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:verifyexceptionthrown.pro:<TRUE>:
# CONFIG = "-debug_and_release_target" "exceptions"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:verifyexceptionthrown.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
