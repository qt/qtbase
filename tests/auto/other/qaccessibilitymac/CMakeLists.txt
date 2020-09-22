# Generated from qaccessibilitymac.pro.

if(NOT APPLE)
    return()
endif()

#####################################################################
## tst_qaccessibilitymac Test:
#####################################################################

qt_internal_add_test(tst_qaccessibilitymac
    SOURCES
        tst_qaccessibilitymac.cpp
        tst_qaccessibilitymac_helpers.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:qaccessibilitymac.pro:<TRUE>:
# _REQUIREMENTS = "mac"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qaccessibilitymac CONDITION APPLE
    SOURCES
        tst_qaccessibilitymac_helpers.mm
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWSecurity}
)
