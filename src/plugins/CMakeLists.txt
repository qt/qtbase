# Generated from plugins.pro.


if(TARGET Qt::Sql)
    add_subdirectory(sqldrivers)
endif()

if(TARGET Qt::Network AND QT_FEATURE_bearermanagement)
    add_subdirectory(bearer)
endif()

if(TARGET Qt::Gui)
    add_subdirectory(platforms)
    add_subdirectory(platforminputcontexts)
    add_subdirectory(platformthemes)

    if(QT_FEATURE_imageformatplugin)
        add_subdirectory(imageformats)
    endif()

    if(NOT ANDROID AND QT_FEATURE_library)
        add_subdirectory(generic)
    endif()
endif()

if(TARGET Qt::Widgets)
    add_subdirectory(styles)
endif()

if(NOT WINRT AND TARGET Qt::PrintSupport)
    add_subdirectory(printsupport)
endif()
