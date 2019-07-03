# Generated from qtranslator.pro.

#####################################################################
## tst_qtranslator Test:
#####################################################################

add_qt_test(tst_qtranslator
    SOURCES
        tst_qtranslator.cpp
)

# Resources:
add_qt_resource(tst_qtranslator "qtranslator" PREFIX "/tst_qtranslator" FILES
    hellotr_empty.qm
    hellotr_la.qm)


#### Keys ignored in scope 1:.:.:qtranslator.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

# Resources:
add_qt_resource(tst_qtranslator "android_testdata" PREFIX "/android_testdata" FILES
    dependencies_la.qm
    hellotr_empty.qm
    hellotr_la.qm
    msgfmt_from_po.qm)


#### Keys ignored in scope 3:.:.:qtranslator.pro:else:
# TESTDATA = "dependencies_la.qm" "hellotr_empty.qm" "hellotr_la.qm" "msgfmt_from_po.qm"
