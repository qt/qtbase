# Generated from plugin.pro.

if(QT_BUILD_SHARED_LIBS)
    add_subdirectory(qfactoryloader)
endif()
add_subdirectory(quuid)
if(QT_FEATURE_library)
    add_subdirectory(qpluginloader)
    add_subdirectory(qlibrary)
endif()
if(QT_BUILD_SHARED_LIBS AND QT_FEATURE_library)
    add_subdirectory(qplugin)
endif()
