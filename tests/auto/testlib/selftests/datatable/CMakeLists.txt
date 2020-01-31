# Generated from datatable.pro.

#####################################################################
## datatable Binary:
#####################################################################

add_qt_executable(datatable
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_datatable.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
