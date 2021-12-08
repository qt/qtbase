qt_add_plugin(pnp_basictools
    STATIC
    CLASS_NAME BasicToolsPlugin
)

target_sources(pnp_basictools PRIVATE
    basictoolsplugin.cpp basictoolsplugin.h
)

target_include_directories(pnp_basictools PUBLIC
    ../../app
)

target_link_libraries(pnp_basictools PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
