# Generated from network.pro.

# special case begin
# SSL library include path is not propagated with private tests which results in
# test not being able to find the ssl header when they are not in the standard
# include paths
if (QT_FEATURE_private_tests)
    if (QT_FEATURE_openssl AND QT_FEATURE_openssl_linked AND QT_FEATURE_ssl)
        include_directories($<TARGET_PROPERTY:OpenSSL::SSL,INTERFACE_INCLUDE_DIRECTORIES>)
    endif()

    if (QT_FEATURE_openssl AND QT_FEATURE_ssl AND NOT QT_FEATURE_openssl_linked)
        include_directories($<TARGET_PROPERTY:WrapOpenSSLHeaders::WrapOpenSSLHeaders,INTERFACE_INCLUDE_DIRECTORIES>)
    endif()
endif()
# special case end

add_subdirectory(access)
add_subdirectory(kernel)
add_subdirectory(socket)
if(QT_FEATURE_openssl)
    add_subdirectory(ssl)
endif()
