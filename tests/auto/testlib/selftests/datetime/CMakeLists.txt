# Generated from datetime.pro.

#####################################################################
## datetime Binary:
#####################################################################

add_qt_executable(datetime
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_datetime.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
