# Generated from qpolygon.pro.

#####################################################################
## tst_qpolygon Test:
#####################################################################

qt_add_test(tst_qpolygon
    SOURCES
        tst_qpolygon.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_extend_target(tst_qpolygon CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    PUBLIC_LIBRARIES
        m
)
