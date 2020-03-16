# Generated from widgets.pro.

add_subdirectory(animation)
add_subdirectory(desktop)
add_subdirectory(dialogs)
if(QT_FEATURE_draganddrop) # special case
   add_subdirectory(draganddrop)
endif()
add_subdirectory(effects)
add_subdirectory(gestures)
add_subdirectory(graphicsview)
add_subdirectory(itemviews)
add_subdirectory(layouts)
add_subdirectory(painting)
add_subdirectory(richtext)
add_subdirectory(scroller)
add_subdirectory(statemachine)
add_subdirectory(tools)
add_subdirectory(touch)
add_subdirectory(tutorials)
add_subdirectory(widgets)

if(TARGET Qt::Gui AND QT_FEATURE_opengl)
    add_subdirectory(windowcontainer)
endif()

if(QT_FEATURE_cursor) # special case
    add_subdirectory(mainwindows)
endif()

if(MACOS)
    add_subdirectory(mac)
endif()
