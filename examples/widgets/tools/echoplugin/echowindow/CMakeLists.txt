qt_add_executable(echopluginwindow
    echointerface.h
    echowindow.cpp echowindow.h
    main.cpp
)
set_target_properties(echopluginwindow PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(echopluginwindow PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

if(NOT QT6_IS_SHARED_LIBS_BUILD)
    target_link_libraries(echopluginwindow PRIVATE
        echoplugin
    )
endif()

install(TARGETS echopluginwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
