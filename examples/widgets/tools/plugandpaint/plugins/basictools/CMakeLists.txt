# special case begin

add_library(pnp_basictools MODULE STATIC
    basictoolsplugin.cpp basictoolsplugin.h
)
set_target_properties(pnp_basictools PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "../../plugins"
        RUNTIME_OUTPUT_DIRECTORY "../../plugins"
)
target_include_directories(pnp_basictools PUBLIC ../../app)
target_link_libraries(pnp_basictools PUBLIC Qt::Widgets)

# special case end
