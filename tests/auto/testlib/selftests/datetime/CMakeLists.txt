# Generated from datetime.pro.

#####################################################################
## datetime Binary:
#####################################################################

add_qt_executable(datetime
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_datetime.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:datetime.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:datetime.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
