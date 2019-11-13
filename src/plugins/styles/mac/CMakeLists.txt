# Generated from mac.pro.

#####################################################################
## qmacstyle Plugin:
#####################################################################

qt_add_plugin(qmacstyle
    TYPE styles
    CLASS_NAME QMacStylePlugin
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
