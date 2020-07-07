# Generated from embedded.pro.

if(NOT TARGET Qt::Gui OR (NOT embedded AND NOT x11))
    return()
endif()
add_subdirectory(styleexample)
add_subdirectory(raycasting)
add_subdirectory(flickable)
add_subdirectory(digiflip)
add_subdirectory(lightmaps)
add_subdirectory(flightinfo)
