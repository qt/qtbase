# Generated from qglyphrun.pro.

#####################################################################
## tst_qglyphrun Test:
#####################################################################

add_qt_test(tst_qglyphrun
    SOURCES
        tst_qglyphrun.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

# Resources:
set_source_files_properties("../../../shared/resources/test.ttf"
    PROPERTIES QT_RESOURCE_ALIAS "test.ttf"
)
set(testdata_resource_files
    "../../../shared/resources/test.ttf"
)

add_qt_resource(tst_qglyphrun "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

