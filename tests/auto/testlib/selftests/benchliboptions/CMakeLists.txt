# Generated from benchliboptions.pro.

#####################################################################
## benchliboptions Binary:
#####################################################################

add_qt_executable(benchliboptions
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_benchliboptions.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:benchliboptions.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:benchliboptions.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
