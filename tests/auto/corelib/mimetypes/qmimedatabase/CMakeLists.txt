# Generated from qmimedatabase.pro.

if(TARGET Qt::Concurrent)
    add_subdirectory(qmimedatabase-xml)
endif()
if(TARGET Qt::Concurrent AND UNIX AND NOT APPLE AND NOT QNX)
    add_subdirectory(qmimedatabase-cache)
endif()
