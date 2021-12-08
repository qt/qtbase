qt_add_plugin(pnp_extrafilters
    CLASS_NAME ExtraFiltersPlugin
)

target_sources(pnp_extrafilters PRIVATE
    extrafiltersplugin.cpp extrafiltersplugin.h
)

set_target_properties(pnp_extrafilters PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/app"
)

target_include_directories(pnp_extrafilters PUBLIC
    ../../app
)

target_link_libraries(pnp_extrafilters PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS pnp_extrafilters
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}/plugins"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}/plugins"
)
