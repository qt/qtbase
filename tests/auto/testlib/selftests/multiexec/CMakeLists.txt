# Generated from multiexec.pro.

#####################################################################
## multiexec Binary:
#####################################################################

add_qt_executable(multiexec
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_multiexec.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:multiexec.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:multiexec.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
