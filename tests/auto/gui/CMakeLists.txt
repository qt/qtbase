# Generated from gui.pro.

add_subdirectory(kernel)
if(NOT APPLE_UIKIT)
    add_subdirectory(image)
    add_subdirectory(math3d)
    add_subdirectory(painting)
    # add_subdirectory(text) # special case missing project
    add_subdirectory(util)
    add_subdirectory(itemmodels)
    add_subdirectory(rhi)
endif()
if(QT_FEATURE_opengl AND NOT APPLE_UIKIT AND NOT WINRT)
    # add_subdirectory(qopenglconfig) # special case missing project
    # add_subdirectory(qopengl) # special case missing project
endif()
if(QT_FEATURE_vulkan AND NOT APPLE_UIKIT)
    # add_subdirectory(qvulkan) # special case missing project
endif()
