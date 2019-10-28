# Generated from painting.pro.

add_subdirectory(drawtexture)
add_subdirectory(qcolor)
add_subdirectory(qregion)
add_subdirectory(qtransform)
add_subdirectory(lancebench)
if(TARGET Qt::Widgets)
    add_subdirectory(qpainter)
    add_subdirectory(qtbench)
endif()
