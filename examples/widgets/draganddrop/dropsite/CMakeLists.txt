cmake_minimum_required(VERSION 3.16)
project(dropsite LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/dropsite")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(dropsite
    droparea.cpp droparea.h
    dropsitewindow.cpp dropsitewindow.h
    main.cpp
)

set_target_properties(dropsite PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(dropsite PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS dropsite
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
