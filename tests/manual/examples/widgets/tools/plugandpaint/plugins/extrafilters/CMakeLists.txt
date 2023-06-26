# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_plugin(pnp_extrafilters
    CLASS_NAME ExtraFiltersPlugin
    extrafiltersplugin.cpp extrafiltersplugin.h
)

set_target_properties(pnp_extrafilters PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app"
)

target_include_directories(pnp_extrafilters PRIVATE
    ../../app
)

target_link_libraries(pnp_extrafilters PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

install(TARGETS pnp_extrafilters
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}/plugins"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}/plugins"
)
