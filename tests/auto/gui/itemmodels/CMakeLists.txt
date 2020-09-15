# Generated from itemmodels.pro.

add_subdirectory(qstandarditem)
if(TARGET Qt::Widgets)
    add_subdirectory(qstandarditemmodel)
endif()
if(TARGET Qt::Widgets AND NOT MINGW)
    add_subdirectory(qfilesystemmodel)
endif()
