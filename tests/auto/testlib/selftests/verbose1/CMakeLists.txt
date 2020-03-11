# Generated from verbose1.pro.

#####################################################################
## verbose1 Binary:
#####################################################################

add_qt_executable(verbose1
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        ../counting/tst_counting.cpp
    DEFINES
        TESTLIB_VERBOSITY_ARG="-v1"
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
