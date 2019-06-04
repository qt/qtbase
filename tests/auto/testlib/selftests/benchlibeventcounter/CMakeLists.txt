# Generated from benchlibeventcounter.pro.

#####################################################################
## benchlibeventcounter Binary:
#####################################################################

add_qt_executable(benchlibeventcounter
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_benchlibeventcounter.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:benchlibeventcounter.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:benchlibeventcounter.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
