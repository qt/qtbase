# Generated from test.pro.

#####################################################################
## tst_qfactoryloader Test:
#####################################################################

add_qt_test(tst_qfactoryloader
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../plugin1/plugininterface1.h
        ../plugin2/plugininterface2.h
        ../tst_qfactoryloader.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

# special case begin
if (NOT QT_FEATURE_library)
    target_link_directories(tst_qfactoryloader PRIVATE "${CMAKE_CURRENT_BINARY_DIR}../bin")
endif()
# special case end

extend_target(tst_qfactoryloader CONDITION NOT QT_FEATURE_library
    PUBLIC_LIBRARIES
        # Remove: L
        # special case begin
        # this part is handled as a special case above
        # ../bin/
        # special case end
)

if(ANDROID)
    # special case begin
    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/../bin/libplugin1.so
        PROPERTIES QT_RESOURCE_TARGET_DEPENDENCY plugin1
    )

    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/../bin/libplugin2.so
        PROPERTIES QT_RESOURCE_TARGET_DEPENDENCY plugin2
    )
    # Resources:
    if (ANDROID)
        set(qmake_libs_resource_files
            bin/libplugin1_${CMAKE_ANDROID_ARCH_ABI}.so
            bin/libplugin2_${CMAKE_ANDROID_ARCH_ABI}.so
        )
    else()
        set(qmake_libs_resource_files
            bin/libplugin1.so
            bin/libplugin2.so
        )
    endif()
    # special case end

    add_qt_resource(tst_qfactoryloader "qmake_libs"
        PREFIX
            "android_test_data"
        BASE
            "${CMAKE_CURRENT_BINARY_DIR}/.."
        FILES
            ${qmake_libs_resource_files}
    )
endif()
