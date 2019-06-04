if(NOT WINRT)
    add_qt_executable(qlogging_helper
        NO_INSTALL # special case
        OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
        SOURCES app/main.cpp
        DEFINES QT_MESSAGELOGCONTEXT
        LIBRARIES Qt::Core)
endif()

add_qt_test(tst_qlogging SOURCES tst_qlogging.cpp
    DEFINES
        QT_MESSAGELOGCONTEXT
        QT_DISABLE_DEPRECATED_BEFORE=0
        HELPER_BINARY="${CMAKE_CURRENT_BINARY_DIR}/qlogging_helper" # special case
)
