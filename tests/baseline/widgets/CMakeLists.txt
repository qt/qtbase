qt_internal_add_test(tst_baseline_widgets
    SOURCES
        ../shared/baselineprotocol.cpp ../shared/baselineprotocol.h ../shared/lookup3.cpp
        ../shared/qbaselinetest.cpp ../shared/qbaselinetest.h
        ../shared/qwidgetbaselinetest.cpp ../shared/qwidgetbaselinetest.h
        tst_baseline_widgets.cpp
    INCLUDE_DIRECTORIES
        ../shared
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
        Qt::Network
)
