# Generated from qabstractitemmodeltester.pro.

#####################################################################
## qabstractitemmodeltester Test:
#####################################################################

qt_internal_add_test(qabstractitemmodeltester
    SOURCES
        ../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../other/qabstractitemmodelutils/dynamictreemodel.h
        tst_qabstractitemmodeltester.cpp
    INCLUDE_DIRECTORIES
        ../../other/qabstractitemmodelutils
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
