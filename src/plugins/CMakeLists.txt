if(QT_FEATURE_gui)
    add_subdirectory(imageformats)
    add_subdirectory(platforms)

    if (NOT ANDROID AND QT_FEATURE_library)
        add_subdirectory(generic)
    endif()
endif()
