qt_add_plugin(simplestyleplugin
    CLASS_NAME SimpleStylePlugin
)
target_sources(simplestyleplugin PRIVATE
    simplestyle.cpp simplestyle.h
    simplestyleplugin.cpp simplestyleplugin.h
)
set_target_properties(simplestyleplugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/stylewindow/styles"
)
target_link_libraries(simplestyleplugin PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS simplestyleplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
