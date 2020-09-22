# Generated from qglyphrun.pro.

#####################################################################
## tst_qglyphrun Test:
#####################################################################

qt_internal_add_test(tst_qglyphrun
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

qt_internal_add_resource(tst_qglyphrun "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
)

