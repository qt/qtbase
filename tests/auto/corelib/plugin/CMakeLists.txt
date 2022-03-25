# Generated from plugin.pro.

# QTBUG-87438 # special case
if(QT_BUILD_SHARED_LIBS AND NOT ANDROID)
    add_subdirectory(qfactoryloader)
endif()
add_subdirectory(quuid)
# QTBUG-87438 # special case
if(QT_FEATURE_library AND NOT ANDROID)
    add_subdirectory(qpluginloader)
    add_subdirectory(qlibrary)
endif()
if(QT_BUILD_SHARED_LIBS AND QT_FEATURE_library)
    add_subdirectory(qplugin)
endif()
