qt_find_package(WrapFreetype PROVIDED_TARGETS WrapFreetype::WrapFreetype)

qt_internal_add_plugin(QVkKhrDisplayIntegrationPlugin
    OUTPUT_NAME qvkkhrdisplay
    PLUGIN_TYPE platforms
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES vkkhrdisplay
    SOURCES
        main.cpp
        qvkkhrdisplayintegration.cpp qvkkhrdisplayintegration.h
        qvkkhrdisplayvulkaninstance.cpp qvkkhrdisplayvulkaninstance.h
    DEFINES
        QT_NO_FOREACH
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::FbSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_extend_target(QVkKhrDisplayIntegrationPlugin CONDITION QT_FEATURE_freetype
    LIBRARIES
        WrapFreetype::WrapFreetype
)

qt_internal_extend_target(QVkKhrDisplayIntegrationPlugin CONDITION TARGET Qt::InputSupportPrivate
    LIBRARIES
        Qt::InputSupportPrivate
)
