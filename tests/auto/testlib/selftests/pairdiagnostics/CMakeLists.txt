# Generated from pairdiagnostics.pro.

#####################################################################
## pairdiagnostics Binary:
#####################################################################

add_qt_executable(pairdiagnostics
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_pairdiagnostics.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)
