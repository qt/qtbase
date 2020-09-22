# Generated from qimagereader.pro.

#####################################################################
## tst_qimagereader Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    baseline/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qimagereader
    SOURCES
        tst_qimagereader.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
    TESTDATA ${test_data}
)

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "images/*")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_immediate_resource_files
    ${resource_glob_0}
)

qt_internal_add_resource(tst_qimagereader "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)


#### Keys ignored in scope 1:.:.:qimagereader.pro:<TRUE>:
# MOC_DIR = "tmp"

## Scopes:
#####################################################################

if(ANDROID AND NOT ANDROID_EMBEDDED)
    # Resources:
    set(android_testdata_resource_files
        "images/kollada-noext"
        "images/trans.gif"
    )

    qt_internal_add_resource(tst_qimagereader "android_testdata"
        PREFIX
            "/"
        FILES
            ${android_testdata_resource_files}
    )
endif()
