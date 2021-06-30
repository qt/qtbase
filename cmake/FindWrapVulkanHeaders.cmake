# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapVulkanHeaders::WrapVulkanHeaders)
    set(WrapVulkanHeaders_FOUND ON)
    return()
endif()

set(WrapVulkanHeaders_FOUND OFF)

find_package(Vulkan ${WrapVulkanHeaders_FIND_VERSION} QUIET)

# We are interested only in include headers. The libraries might be missing, so we can't check the
# _FOUND variable.
if(Vulkan_INCLUDE_DIR)
    set(WrapVulkanHeaders_FOUND ON)

    add_library(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE IMPORTED)
    target_include_directories(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE
        ${Vulkan_INCLUDE_DIR})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapVulkanHeaders DEFAULT_MSG Vulkan_INCLUDE_DIR)
