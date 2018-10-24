if(NOT WINRT)
    add_qt_test_helper(qlogging_helper SOURCES app/main.cpp DEFINES QT_MESSAGELOGCONTEXT LIBRARIES Qt::Core)
endif()

add_qt_test(tst_qlogging SOURCES tst_qlogging.cpp
    DEFINES
        QT_MESSAGELOGCONTEXT
        QT_DISABLE_DEPRECATED_BEFORE=0
        HELPER_BINARY="qlogging_helper"
)
