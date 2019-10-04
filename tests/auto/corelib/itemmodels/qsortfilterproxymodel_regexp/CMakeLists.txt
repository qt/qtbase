# Generated from qsortfilterproxymodel_regexp.pro.

#####################################################################
## tst_qsortfilterproxymodel_regexp Test:
#####################################################################

add_qt_test(tst_qsortfilterproxymodel_regexp
    SOURCES
        ../../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../../other/qabstractitemmodelutils/dynamictreemodel.h
        ../qsortfilterproxymodel_common/tst_qsortfilterproxymodel.cpp ../qsortfilterproxymodel_common/tst_qsortfilterproxymodel.h
        tst_qsortfilterproxymodel_regexp.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
        ../qsortfilterproxymodel_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
