# Generated from qcolorspace.pro.

#####################################################################
## tst_qcolorspace Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    resources/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qcolorspace
    SOURCES
        tst_qcolorspace.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "resources/*")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_immediate_resource_files
    ${resource_glob_0}
)

qt_internal_add_resource(tst_qcolorspace "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

