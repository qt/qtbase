# Generated from qtransform.pro.

#####################################################################
## tst_qtransform Test:
#####################################################################

qt_internal_add_test(tst_qtransform
    SOURCES
        tst_qtransform.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qtransform CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    PUBLIC_LIBRARIES
        m
)
