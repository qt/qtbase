# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

set_target_properties(QMacStylePlugin
    PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
)
