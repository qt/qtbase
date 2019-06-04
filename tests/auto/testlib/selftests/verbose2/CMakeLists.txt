# Generated from verbose2.pro.

#####################################################################
## verbose2 Binary:
#####################################################################

add_qt_executable(verbose2
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        ../counting/tst_counting.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:verbose2.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:verbose2.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
