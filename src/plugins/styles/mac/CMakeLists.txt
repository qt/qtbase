# Generated from mac.pro.

#####################################################################
## QMacStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QMacStylePlugin
    OUTPUT_NAME qmacstyle
    PLUGIN_TYPE styles
    SOURCES
        main.mm
        qmacstyle_mac.mm qmacstyle_mac_p.h
        qmacstyle_mac_p_p.h
    LIBRARIES
        ${FWAppKit}
        Qt::Core
        Qt::Gui
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:mac.pro:<TRUE>:
# DISTFILES = "macstyle.json"

# special case begin
set_target_properties(QMacStylePlugin
    PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
)
# special case end
