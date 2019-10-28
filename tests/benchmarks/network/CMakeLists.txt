# Generated from network.pro.

add_subdirectory(access)
add_subdirectory(kernel)
add_subdirectory(socket)
if(QT_FEATURE_openssl)
    add_subdirectory(ssl)
endif()
