#####################################################################
## tst_qaddpreroutine Test:
#####################################################################
cmake_minimum_required(VERSION 3.16)

if(DEFINED CMAKE_Core_MODULE_MAJOR_VERSION)
    set(project_version "${CMAKE_Core_MODULE_MAJOR_VERSION}.\
${CMAKE_Core_MODULE_MINOR_VERSION}.${CMAKE_Core_MODULE_PATCH_VERSION}"
    )
else()
    set(project_version "6.0.0")
endif()

project(tst_qaddpreroutine
        LANGUAGES CXX
        VERSION "${project_version}"
)

find_package(Qt6 COMPONENTS Core BuildInternals CONFIG REQUIRED)
qt_prepare_standalone_project()

find_package(Qt6 COMPONENTS Gui Test CONFIG REQUIRED)

qt_internal_add_plugin(QTBUG_90341ThemePlugin
    OUTPUT_NAME QTBUG_90341
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    TYPE platformthemes
    DEFAULT_IF FALSE
    SOURCES
        plugin.cpp
    SKIP_INSTALL
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
)

qt_internal_add_test(tst_qaddpreroutine
    SOURCES
        tst_qaddpreroutine.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

qt6_import_plugins(tst_qaddpreroutine INCLUDE QTBUG_90341)

target_compile_definitions(tst_qaddpreroutine
    PRIVATE QT_QPA_PLATFORM_PLUGIN_PATH=\"${CMAKE_BINARY_DIR}\")
