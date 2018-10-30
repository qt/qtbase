# Generated from platformsupport.pro.

qt_pull_features_into_current_scope(PRIVATE_FEATURES Qt::Gui)
add_subdirectory(themes)
add_subdirectory(edid)
add_subdirectory(eventdispatchers)

if(QT_FEATURE_freetype OR APPLE OR WIN32)
    add_subdirectory(fontdatabases)
endif()

if((UNIX AND (NOT uikit)) OR QT_FEATURE_xcb)
    add_subdirectory(services)
endif()

if(QT_FEATURE_accessibility)
    add_subdirectory(accessibility)
    if(QT_FEATURE_accessibility_atspi_bridge)
        add_subdirectory(linuxaccessibility)
    endif()
endif()

if(APPLE)
    add_subdirectory(clipboard)
    add_subdirectory(graphics)
endif()
