#####################################################################
## tst_qsortfilterproxymodel Test:
#####################################################################

qt_internal_add_test(tst_qsortfilterproxymodel
    SOURCES
        ../../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../../other/qabstractitemmodelutils/dynamictreemodel.h
        tst_qsortfilterproxymodel.cpp
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
    LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::TestPrivate
)
