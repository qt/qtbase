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
# QTBUG-87438 # special case
if(QT_BUILD_SHARED_LIBS AND QT_FEATURE_library AND NOT ANDROID)
    # special case begin
    # QTBUG-85364
    if(NOT LINUX)
        add_subdirectory(qplugin)
    endif()
    # special case end
endif()
