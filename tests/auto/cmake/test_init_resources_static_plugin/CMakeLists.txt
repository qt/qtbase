cmake_minimum_required(VERSION 3.16)

if(DEFINED CMAKE_Core_MODULE_MAJOR_VERSION)
    set(project_version "${CMAKE_Core_MODULE_MAJOR_VERSION}.\
${CMAKE_Core_MODULE_MINOR_VERSION}.${CMAKE_Core_MODULE_PATCH_VERSION}"
    )
else()
    set(project_version "6.0.0")
endif()

project(TestInitResourcesStaticPlugin
        LANGUAGES CXX
        VERSION "${project_version}"
)

find_package(Qt6 COMPONENTS Core BuildInternals CONFIG REQUIRED)
qt_prepare_standalone_project()

find_package(Qt6 COMPONENTS Gui Test CONFIG REQUIRED) # Add gui since Core have no plugin types

qt_internal_add_plugin(TestInitResourcesStaticPlugin STATIC
    OUTPUT_NAME
        testinitresourcesstaticplugin
    TYPE generic
    SOURCES
        pluginmain.cpp
    SKIP_INSTALL
    LIBRARIES
        Qt::Core
)

qt_internal_add_resource(TestInitResourcesStaticPlugin "teststaticplugin1"
    PREFIX
        "/teststaticplugin1"
    FILES
        "testfile1.txt"
)

qt_internal_add_resource(TestInitResourcesStaticPlugin "teststaticplugin2"
    PREFIX
        "/teststaticplugin2"
    FILES
        "testfile2.txt"
)

qt_internal_add_test(test_init_resources_static_plugin
    SOURCES
        test_init_resources_static_plugin.cpp
    LIBRARIES
        Qt::Core
        TestInitResourcesStaticPlugin
)
