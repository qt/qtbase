# Generated from gui.pro.

add_subdirectory(analogclock)
add_subdirectory(rasterwindow)

if(TARGET Qt::Gui AND QT_FEATURE_opengl)
    add_subdirectory(openglwindow)
endif()
