# Generated from testlib.pro.

#####################################################################
## testlib Binary:
#####################################################################

add_qt_executable(testlib
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_testlib.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:testlib.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:testlib.pro:APPLE:
# CONFIG = "-app_bundle"
