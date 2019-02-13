# Generated from mac.pro.

#####################################################################
## qmacstyle Plugin:
#####################################################################

add_qt_plugin(qmacstyle
    TYPE styles
    SOURCES
        main.mm
        qmacstyle_mac.mm qmacstyle_mac_p.h
        qmacstyle_mac_p_p.h
    LIBRARIES
        Qt::WidgetsPrivate
        ${FWAppKit}
    # DISTFILES = "macstyle.json"
    # PLUGIN_CLASS_NAME = "QMacStylePlugin"
    # _LOADED = "qt_plugin"
)
