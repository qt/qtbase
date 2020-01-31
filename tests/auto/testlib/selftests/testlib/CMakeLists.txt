# Generated from testlib.pro.

#####################################################################
## testlib Binary:
#####################################################################

add_qt_executable(testlib
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_testlib.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
