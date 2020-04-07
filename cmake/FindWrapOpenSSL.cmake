# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapOpenSSL::WrapOpenSSL)
    set(WrapOpenSSL_FOUND ON)
    return()
endif()

set(WrapOpenSSL_FOUND OFF)

# Reuse logic from the headers find script.
find_package(WrapOpenSSLHeaders ${WrapOpenSSL_FIND_VERSION})

if(OpenSSL_FOUND)
    set(WrapOpenSSL_FOUND ON)

    add_library(WrapOpenSSL::WrapOpenSSL INTERFACE IMPORTED)
    target_link_libraries(WrapOpenSSL::WrapOpenSSL INTERFACE OpenSSL::SSL)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapOpenSSL DEFAULT_MSG WrapOpenSSL_FOUND)
