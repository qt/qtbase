# Generated from plugin.pro.

if(QT_BUILD_SHARED_LIBS)
    add_subdirectory(qfactoryloader)
endif()
add_subdirectory(quuid)
if(QT_FEATURE_library)
    # QTBUG-87438 # special case
    if(NOT ANDROID)
        add_subdirectory(qpluginloader)
    endif()
    add_subdirectory(qlibrary)
endif()
if(QT_BUILD_SHARED_LIBS AND QT_FEATURE_library)
    # special case begin
    # QTBUG-85364
    if(NOT LINUX)
        add_subdirectory(qplugin)
    endif()
    # special case end
endif()
