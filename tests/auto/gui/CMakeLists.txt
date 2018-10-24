# Generated from gui.pro.

add_subdirectory("kernel")

if(NOT APPLE_UIKIT)
    add_subdirectory("image")
    add_subdirectory("math3d")
    add_subdirectory("painting")
    if (QT_FEATURE_opengl AND NOT WINRT)
        # missing CMakeLists.txt: add_subdirectory("qopenglconfig")
        # missing CMakeLists.txt: add_subdirectory("qopengl")
    endif()
    # if (QT_FEATURE_vulkan)
    #     add_subdirectory("qvulkan")
    # endif()
    # add_subdirectory("text")
    add_subdirectory("util")
    add_subdirectory("itemmodels")
endif()
