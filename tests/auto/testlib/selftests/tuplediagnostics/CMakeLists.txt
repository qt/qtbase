# Generated from tuplediagnostics.pro.

#####################################################################
## tuplediagnostics Binary:
#####################################################################

add_qt_executable(tuplediagnostics
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_tuplediagnostics.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)
