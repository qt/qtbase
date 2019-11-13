# Generated from failcleanup.pro.

#####################################################################
## failcleanup Binary:
#####################################################################

add_qt_executable(failcleanup
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_failcleanup.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
