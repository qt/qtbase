# special case begin

add_library(pnp_extrafilters_plugin MODULE
    extrafiltersplugin.cpp extrafiltersplugin.h
)
target_include_directories(pnp_extrafilters_plugin PRIVATE ../../app)
target_link_libraries(pnp_extrafilters_plugin PRIVATE Qt::Widgets)

# special case end
