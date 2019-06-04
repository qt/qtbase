# Generated from fetchbogus.pro.

#####################################################################
## fetchbogus Binary:
#####################################################################

add_qt_executable(fetchbogus
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_fetchbogus.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:fetchbogus.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:fetchbogus.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
