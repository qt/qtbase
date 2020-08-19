# Generated from itemmodels.pro.

add_subdirectory(qstandarditem)
if(NOT MINGW)
    add_subdirectory(qfilesystemmodel)
endif()
if(TARGET Qt::Widgets)
    add_subdirectory(qstandarditemmodel)
endif()
