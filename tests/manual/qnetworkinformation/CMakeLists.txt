
qt_internal_add_manual_test(qnetworkinformation
    SOURCES
        tst_qnetworkinformation.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)

qt_internal_extend_target(qnetworkinformation CONDITION ANDROID OR UIKIT
    SOURCES
        mainwindow.h
    PUBLIC_LIBRARIES
        Qt::Widgets
    DEFINES
        MOBILE)
