# Generated from benchlibcallgrind.pro.

#####################################################################
## benchlibcallgrind Binary:
#####################################################################

add_qt_executable(benchlibcallgrind
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_benchlibcallgrind.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:benchlibcallgrind.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:benchlibcallgrind.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
