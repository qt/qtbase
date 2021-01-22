# Generated from qlogging.pro.
# This file is almost completely custom written
# special case skip regeneration

qt_internal_add_executable(qlogging_helper
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES app/main.cpp
    DEFINES QT_MESSAGELOGCONTEXT
    LIBRARIES Qt::Core)

# Fixes required for the backtrace stack to be correct
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT MINGW)
    target_link_options(qlogging_helper PRIVATE -rdynamic)
endif()
set_target_properties(qlogging_helper PROPERTIES CXX_VISIBILITY_PRESET default)

qt_internal_add_test(tst_qlogging SOURCES tst_qlogging.cpp
    DEFINES
        QT_MESSAGELOGCONTEXT
        QT_DISABLE_DEPRECATED_BEFORE=0
        HELPER_BINARY="${CMAKE_CURRENT_BINARY_DIR}/qlogging_helper"
)

qt_internal_add_test(tst_qmessagelogger SOURCES tst_qmessagelogger.cpp
    DEFINES
        QT_MESSAGELOGCONTEXT
        QT_DISABLE_DEPRECATED_BEFORE=0
)
