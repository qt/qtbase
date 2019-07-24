# Generated from network.pro.

add_subdirectory(access)
add_subdirectory(bearer)
add_subdirectory(kernel)
add_subdirectory(ssl)
add_subdirectory(socket)

if(WIN32)
endif()

if(WIN32 OR APPLE)
endif()
