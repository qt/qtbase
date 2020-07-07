# Generated from cmptest.pro.

#####################################################################
## cmptest Binary:
#####################################################################

qt_add_executable(cmptest
    SOURCES
        tst_cmptest.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_extend_target(cmptest CONDITION TARGET Qt::Gui
    PUBLIC_LIBRARIES
        Qt::Gui
)
