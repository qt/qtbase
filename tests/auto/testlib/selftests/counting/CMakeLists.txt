# Generated from counting.pro.

#####################################################################
## counting Binary:
#####################################################################

add_qt_executable(counting
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_counting.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:counting.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:counting.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
