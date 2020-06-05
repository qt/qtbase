add_qt_executable(qlogging_helper
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES app/main.cpp
    DEFINES QT_MESSAGELOGCONTEXT
    LIBRARIES Qt::Core)

#special case begin
# Fixes required for the backtrace stack to be correct
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT MINGW)
    target_link_options(qlogging_helper PRIVATE -rdynamic)
endif()
set_target_properties(qlogging_helper PROPERTIES CXX_VISIBILITY_PRESET default)
# special case end

add_qt_test(tst_qlogging SOURCES tst_qlogging.cpp
    DEFINES
        QT_MESSAGELOGCONTEXT
        QT_DISABLE_DEPRECATED_BEFORE=0
        HELPER_BINARY="${CMAKE_CURRENT_BINARY_DIR}/qlogging_helper" # special case
)

target_compile_definitions(tst_qlogging PRIVATE QT_CMAKE_BUILD) # special case # to fix the binary name
