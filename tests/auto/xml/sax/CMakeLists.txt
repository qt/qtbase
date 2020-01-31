# Generated from sax.pro.

add_subdirectory(qxml)
if(TARGET Qt::Network)
    add_subdirectory(qxmlinputsource)
    add_subdirectory(qxmlsimplereader)
endif()
