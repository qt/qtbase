# Generated from signaldumper.pro.

#####################################################################
## signaldumper Binary:
#####################################################################

add_qt_executable(signaldumper
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_signaldumper.cpp
    LIBRARIES
        Qt::TestPrivate
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
