add_library(widgets_customviewstyle_snippets OBJECT
    ../customstyle/customstyle.h
    ../customstyle/customstyle.cpp
    customviewstyle.cpp
)

target_link_libraries(widgets_customviewstyle_snippets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
