#####################################################################
## tst_qtconcurrentfiltermapgenerated Test:
#####################################################################

qt_internal_add_test(tst_qtconcurrentfiltermapgenerated
    SOURCES
        tst_qtconcurrentfiltermapgenerated.cpp
        tst_qtconcurrentfiltermapgenerated.h
        tst_qtconcurrent_selected_tests.cpp
    PUBLIC_LIBRARIES
        Qt::Concurrent
)
