# Generated from benchlibcounting.pro.

#####################################################################
## benchlibcounting Binary:
#####################################################################

add_qt_executable(benchlibcounting
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_benchlibcounting.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:benchlibcounting.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:benchlibcounting.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
