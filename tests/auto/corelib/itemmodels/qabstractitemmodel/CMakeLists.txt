# Generated from qabstractitemmodel.pro.

#####################################################################
## tst_qabstractitemmodel Test:
#####################################################################

qt_internal_add_test(tst_qabstractitemmodel
    SOURCES
        ../../../other/qabstractitemmodelutils/dynamictreemodel.cpp ../../../other/qabstractitemmodelutils/dynamictreemodel.h
        tst_qabstractitemmodel.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../other/qabstractitemmodelutils
    PUBLIC_LIBRARIES
        Qt::Gui
)
