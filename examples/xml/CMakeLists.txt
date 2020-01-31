# Generated from xml.pro.

add_subdirectory(htmlinfo)
add_subdirectory(xmlstreamlint)

if(TARGET Qt::Widgets)
    add_subdirectory(dombookmarks)
    add_subdirectory(streambookmarks)

    if(TARGET Qt::Network)
        add_subdirectory(rsslisting)
    endif()
endif()
