add_qt_test("tst_qwidgetsvariant"
    SOURCES
        tst_qwidgetsvariant.cpp
    INCLUDE_DIRECTORIES
        ./../../../other/qvariant_common
    LIBRARIES
        Qt::Widgets
)
