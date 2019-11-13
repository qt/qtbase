# Generated from fetchbogus.pro.

#####################################################################
## fetchbogus Binary:
#####################################################################

add_qt_executable(fetchbogus
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_fetchbogus.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
