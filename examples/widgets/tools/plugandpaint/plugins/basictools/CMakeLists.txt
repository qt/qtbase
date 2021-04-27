# Generated from basictools.pro.
# special case skip regeneration

qt_add_plugin(pnp_basictools STATIC)
target_sources(pnp_basictools PRIVATE
    basictoolsplugin.cpp basictoolsplugin.h
)
set_target_properties(pnp_basictools PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(pnp_basictools PUBLIC
    ../../app
)

target_link_libraries(pnp_basictools PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
