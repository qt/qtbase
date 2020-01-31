# Generated from multiexec.pro.

#####################################################################
## multiexec Binary:
#####################################################################

add_qt_executable(multiexec
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_multiexec.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
