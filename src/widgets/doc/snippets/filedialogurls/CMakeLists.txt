add_library(widgets_filedialogurl_snippets OBJECT
    filedialogurls.cpp
)

target_link_libraries(widgets_filedialogurl_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
