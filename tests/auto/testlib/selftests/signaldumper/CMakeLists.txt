# Generated from signaldumper.pro.

#####################################################################
## signaldumper Binary:
#####################################################################

add_qt_executable(signaldumper
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_signaldumper.cpp
    LIBRARIES
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:signaldumper.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:signaldumper.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
