# Generated from qmimedatabase.pro.


if(TARGET Qt::Concurrent)
    add_subdirectory(qmimedatabase-xml)

    if(UNIX AND NOT APPLE AND NOT QNX)
        add_subdirectory(qmimedatabase-cache)
    endif()
endif()
