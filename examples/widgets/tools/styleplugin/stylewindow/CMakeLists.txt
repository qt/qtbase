qt_add_executable(styleplugin
    main.cpp
    stylewindow.cpp stylewindow.h
)
set_target_properties(styleplugin PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(styleplugin PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)
if(NOT QT6_IS_SHARED_LIBS_BUILD)
    target_link_libraries(styleplugin PRIVATE
        simplestyleplugin
    )
endif()

install(TARGETS styleplugin
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
