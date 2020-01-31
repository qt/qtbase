# Generated from qabstractitemmodeltester.pro.

#####################################################################
## qabstractitemmodeltester Test:
#####################################################################

add_qt_test(qabstractitemmodeltester
    SOURCES
        ../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../other/qabstractitemmodelutils/dynamictreemodel.h
        tst_qabstractitemmodeltester.cpp
    INCLUDE_DIRECTORIES
        ../../other/qabstractitemmodelutils
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
