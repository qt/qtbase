# Generated from qpathclipper.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qpathclipper Test:
#####################################################################

qt_internal_add_test(tst_qpathclipper
    SOURCES
        paths.cpp paths.h
        tst_qpathclipper.cpp
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:qpathclipper.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpathclipper CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    PUBLIC_LIBRARIES
        m
)
