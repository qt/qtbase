add_library(widgets_mdiarea_snippets OBJECT
    mdiareasnippets.cpp
)

target_link_libraries(widgets_mdiarea_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
