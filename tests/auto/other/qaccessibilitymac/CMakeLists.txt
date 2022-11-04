
if(NOT APPLE)
    return()
endif()

qt_internal_add_test(tst_qaccessibilitymac
    SOURCES
        tst_qaccessibilitymac.mm
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
        ${FWAppKit}
        ${FWApplicationServices}
        ${FWSecurity}
)
