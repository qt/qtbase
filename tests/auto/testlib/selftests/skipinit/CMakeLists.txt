# Generated from skipinit.pro.

#####################################################################
## skipinit Binary:
#####################################################################

add_qt_executable(skipinit
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_skipinit.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:skipinit.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:skipinit.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
