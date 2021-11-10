#####################################################################
## tst_qsortfilterproxymodel_regularexpression Test:
#####################################################################

qt_internal_add_test(tst_qsfpm_regex
    SOURCES
        tst_qsortfilterproxymodel_regularexpression.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
    LIBRARIES
        Qt::TestPrivate
)
