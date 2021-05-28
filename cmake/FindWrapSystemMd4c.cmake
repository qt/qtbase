if(TARGET WrapSystemMd4c::WrapSystemMd4c)
    set(WrapSystemMd4c_FOUND TRUE)
    return()
endif()

find_package(md4c CONFIG)

# md4c provides a md4c::md4c target but
# older versions create a md4c target without
# namespace. If we find the old variant create
# a namespaced target out of the md4c target.
if(TARGET md4c AND NOT TARGET md4c::md4c)
    add_library(md4c::md4c INTERFACE IMPORTED)
    target_link_libraries(md4c::md4c INTERFACE md4c)
endif()

if(TARGET md4c::md4c)
    add_library(WrapSystemMd4c::WrapSystemMd4c INTERFACE IMPORTED)
    target_link_libraries(WrapSystemMd4c::WrapSystemMd4c INTERFACE md4c::md4c)
endif()

if(TARGET WrapSystemMd4c::WrapSystemMd4c)
    set(WrapSystemMd4c_FOUND TRUE)
endif()
