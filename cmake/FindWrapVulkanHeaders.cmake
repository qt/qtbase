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

    # Also propagate MoltenVK include directory on Apple platforms if found.
    # Assumes the folder structure of the LunarG Vulkan SDK.
    if(APPLE)
        set(__qt_molten_vk_include_path "${Vulkan_INCLUDE_DIR}/../../MoltenVK/include")
        get_filename_component(
            __qt_molten_vk_include_path
            "${__qt_molten_vk_include_path}" ABSOLUTE)
        if(EXISTS "${__qt_molten_vk_include_path}")
            target_include_directories(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE
                ${__qt_molten_vk_include_path})
        endif()
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapVulkanHeaders DEFAULT_MSG Vulkan_INCLUDE_DIR)
