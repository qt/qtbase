cmake_minimum_required(VERSION 3.16)
project(shortcuteditor LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/shortcuteditor")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(shortcuteditor
    actionmanager.cpp actionmanager.h
    application.cpp application.h
    main.cpp
    mainwindow.cpp mainwindow.h
    shortcuteditordelegate.cpp shortcuteditordelegate.h
    shortcuteditormodel.cpp shortcuteditormodel.h
    shortcuteditorwidget.cpp shortcuteditorwidget.h
)

set_target_properties(shortcuteditor PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(shortcuteditor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

install(TARGETS shortcuteditor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
