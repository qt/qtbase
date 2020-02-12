# Generated from badxml.pro.

#####################################################################
## badxml Binary:
#####################################################################

add_qt_executable(badxml
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_badxml.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)

## Scopes:
#####################################################################
