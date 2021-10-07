qt_internal_add_plugin(QNLMNIPlugin
    OUTPUT_NAME qnetworklistmanager
    CLASS_NAME QNetworkListManagerNetworkInformationBackendFactory
    PLUGIN_TYPE networkinformation
    DEFAULT_IF WIN32 AND QT_FEATURE_networklistmanager
    SOURCES
        qnetworklistmanagernetworkinformationbackend.cpp
        qnetworklistmanagerevents.h qnetworklistmanagerevents.cpp
    LIBRARIES
        Qt::NetworkPrivate
)

qt_internal_extend_target(QNLMNIPlugin CONDITION WIN32 AND MSVC AND NOT CLANG
    LIBRARIES
        runtimeobject
)

# Don't repeat the target name in AUTOGEN_BUILD_DIR to work around issues with overlong paths.
set_property(TARGET QNLMNIPlugin PROPERTY
    AUTOGEN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/autogen")
