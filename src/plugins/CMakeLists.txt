if(QT_FEATURE_gui)
    add_subdirectory(imageformats)
    add_subdirectory(platforms)
    add_subdirectory(platformthemes)

    if (NOT ANDROID AND QT_FEATURE_library)
        add_subdirectory(generic)
    endif()
endif()
