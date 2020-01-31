# Generated from exceptionthrow.pro.

#####################################################################
## exceptionthrow Binary:
#####################################################################

add_qt_executable(exceptionthrow
    EXCEPTIONS
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_exceptionthrow.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
