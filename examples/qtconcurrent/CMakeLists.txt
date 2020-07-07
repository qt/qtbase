# Generated from qtconcurrent.pro.

if(NOT TARGET Qt::Concurrent)
    return()
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(imagescaling)
    add_subdirectory(progressdialog)
    add_subdirectory(runfunction)
    add_subdirectory(wordcount)
endif()
if(TARGET Qt::Gui)
    add_subdirectory(map)
endif()
