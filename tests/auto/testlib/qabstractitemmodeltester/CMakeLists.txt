add_qt_test(tst_qabstractitemmodeltester SOURCES
        tst_qabstractitemmodeltester.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../other/qabstractitemmodelutils/dynamictreemodel.cpp
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../other/qabstractitemmodelutils
    LIBRARIES
        Qt::Widgets
)
