# Generated from benchlibcounting.pro.

#####################################################################
## benchlibcounting Binary:
#####################################################################

add_qt_executable(benchlibcounting
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_benchlibcounting.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
