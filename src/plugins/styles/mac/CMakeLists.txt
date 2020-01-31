# Generated from mac.pro.

#####################################################################
## QMacStylePlugin Plugin:
#####################################################################

qt_add_plugin(QMacStylePlugin
    OUTPUT_NAME qmacstyle
    TYPE styles
    SOURCES
        main.mm
        qmacstyle_mac.mm qmacstyle_mac_p.h
        qmacstyle_mac_p_p.h
    LIBRARIES
        ${FWAppKit}
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)

#### Keys ignored in scope 1:.:.:mac.pro:<TRUE>:
# DISTFILES = "macstyle.json"
