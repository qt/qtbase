# Generated from qsortfilterproxymodel_regularexpression.pro.

#####################################################################
## tst_qsortfilterproxymodel_regularexpression Test:
#####################################################################

qt_internal_add_test(tst_qsfpm_regex
    SOURCES
        ../../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../../other/qabstractitemmodelutils/dynamictreemodel.h
        ../qsortfilterproxymodel_common/tst_qsortfilterproxymodel.cpp ../qsortfilterproxymodel_common/tst_qsortfilterproxymodel.h
        tst_qsortfilterproxymodel_regularexpression.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
        ../qsortfilterproxymodel_common
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::TestPrivate
)
