# Generated from crashes.pro.

#####################################################################
## crashes Binary:
#####################################################################

add_qt_executable(crashes
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_crashes.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
