# Generated from testapp.pro.

#####################################################################
## testapp Binary:
#####################################################################

qt_internal_add_executable(windeploy_testapp # special case
    GUI
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

# special case begin
set_target_properties(windeploy_testapp
    PROPERTIES
        OUTPUT_NAME testapp
)
# special case end
