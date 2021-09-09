# Generated from qtranslator.pro.

#####################################################################
## tst_qtranslator Test:
#####################################################################

qt_internal_add_test(tst_qtranslator
    SOURCES
        tst_qtranslator.cpp
)

# Resources:
set(qtranslator_resource_files
    "dependencies_la.qm"
    "hellotr_empty.qm"
    "hellotr_la.qm"
    "msgfmt_from_po.qm"
)

qt_internal_add_resource(tst_qtranslator "qtranslator"
    PREFIX
        "/tst_qtranslator"
    FILES
        ${qtranslator_resource_files}
)


## Scopes:
#####################################################################

if(ANDROID)
    # Resources:
    set(android_testdata_resource_files
        "dependencies_la.qm"
        "hellotr_empty.qm"
        "hellotr_la.qm"
        "msgfmt_from_po.qm"
    )

    qt_internal_add_resource(tst_qtranslator "android_testdata"
        PREFIX
            "/android_testdata"
        FILES
            ${android_testdata_resource_files}
    )
endif()

#### Keys ignored in scope 3:.:.:qtranslator.pro:else:
# TESTDATA = "dependencies_la.qm" "hellotr_empty.qm" "hellotr_la.qm" "msgfmt_from_po.qm"
