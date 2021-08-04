qt_internal_add_plugin(TestInitResourcesStaticPlugin STATIC
    OUTPUT_NAME
        testinitresourcesstaticplugin
    PLUGIN_TYPE mockstaticresources
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
