# Generated from cmptest.pro.

#####################################################################
## cmptest Binary:
#####################################################################

add_qt_executable(cmptest
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_cmptest.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

extend_target(cmptest CONDITION TARGET Qt::Gui
    PUBLIC_LIBRARIES
        Qt::Gui
)
