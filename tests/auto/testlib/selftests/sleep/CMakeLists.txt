# Generated from sleep.pro.

#####################################################################
## sleep Binary:
#####################################################################

add_qt_executable(sleep
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_sleep.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
