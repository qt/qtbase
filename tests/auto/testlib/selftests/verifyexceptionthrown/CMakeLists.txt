# Generated from verifyexceptionthrown.pro.

#####################################################################
## verifyexceptionthrown Binary:
#####################################################################

add_qt_executable(verifyexceptionthrown
    EXCEPTIONS
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_verifyexceptionthrown.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
