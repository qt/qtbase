# Generated from strcmp.pro.

#####################################################################
## strcmp Binary:
#####################################################################

add_qt_executable(strcmp
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_strcmp.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
