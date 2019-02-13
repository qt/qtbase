
if (QT_FEATURE_sql)
    add_subdirectory(sqldrivers)
endif()

if(QT_FEATURE_network AND QT_FEATURE_bearermanagement)
    add_subdirectory(bearer)
endif()

if(QT_FEATURE_gui)
    add_subdirectory(platforms)
    add_subdirectory(platformthemes)
    add_subdirectory(platforminputcontexts)
    if (QT_FEATURE_imageformatplugin)
        add_subdirectory(imageformats)
    endif()
    if (NOT ANDROID AND QT_FEATURE_library)
        add_subdirectory(generic)
    endif()
endif()

if (QT_FEATURE_widgets)
    add_subdirectory(styles)
endif()

if (NOT WINRT AND TARGET PrintSupport)
    add_subdirectory(printsupport)
endif()
