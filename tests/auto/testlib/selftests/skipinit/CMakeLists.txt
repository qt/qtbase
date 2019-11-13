# Generated from skipinit.pro.

#####################################################################
## skipinit Binary:
#####################################################################

add_qt_executable(skipinit
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_skipinit.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
