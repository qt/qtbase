# Generated from embedded.pro.

if(NOT TARGET Qt::Gui OR (NOT embedded AND NOT x11))
    return()
endif()
qt_internal_add_example(styleexample)
qt_internal_add_example(raycasting)
qt_internal_add_example(flickable)
qt_internal_add_example(digiflip)
qt_internal_add_example(lightmaps)
qt_internal_add_example(flightinfo)
