# special case begin

add_library(echoplugin MODULE
    echoplugin.cpp echoplugin.h
)

target_include_directories(echoplugin PUBLIC ../echowindow)
target_link_libraries(echoplugin PUBLIC Qt::Widgets)

# special case end
