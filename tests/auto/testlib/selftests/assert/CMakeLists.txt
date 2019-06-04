# Generated from assert.pro.

#####################################################################
## assert Binary:
#####################################################################

add_qt_executable(assert
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_assert.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:assert.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:assert.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
