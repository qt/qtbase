set(QT_ENABLE_VERBOSE_DEPLOYMENT TRUE)

find_package(Qt6 REQUIRED COMPONENTS Gui)

qt_add_executable(app main.cpp)
target_link_libraries(app PRIVATE Qt6::Gui)

install(TARGETS app RUNTIME DESTINATION bin)

# Copy the qpa plugin to the installation dir so that windeployqt
# attempts to overwrite it.
set(extra_plugin Qt6::QWindowsIntegrationPlugin)
install(FILES "$<TARGET_FILE:${extra_plugin}>"
    DESTINATION "plugins/platforms"
)

# Pass the copied plugin to windeployqt as an extra binary.
qt_generate_deploy_app_script(
    TARGET app
    OUTPUT_SCRIPT deploy_script
    DEPLOY_TOOL_OPTIONS "plugins/platforms/$<TARGET_FILE_NAME:${extra_plugin}>"
)
install(SCRIPT ${deploy_script})
