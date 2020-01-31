# Generated from qaccessibilitymac.pro.

if(NOT APPLE)
    return()
endif()

#####################################################################
## tst_qaccessibilitymac Test:
#####################################################################

add_qt_test(tst_qaccessibilitymac
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

extend_target(tst_qaccessibilitymac CONDITION APPLE
    SOURCES
        tst_qaccessibilitymac_helpers.mm
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWSecurity}
)
