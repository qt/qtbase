# Generated from failinit.pro.

#####################################################################
## failinit Binary:
#####################################################################

add_qt_executable(failinit
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_failinit.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
