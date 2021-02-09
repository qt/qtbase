add_library(widgets_graphicssceneadditem_snippets OBJECT
    graphicssceneadditemsnippet.cpp
)

target_link_libraries(widgets_graphicssceneadditem_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
