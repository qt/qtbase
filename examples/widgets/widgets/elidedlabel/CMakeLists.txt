cmake_minimum_required(VERSION 3.16)
project(elidedlabel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/widgets/elidedlabel")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(elidedlabel
    elidedlabel.cpp elidedlabel.h
    main.cpp
    testwidget.cpp testwidget.h
)

set_target_properties(elidedlabel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(elidedlabel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS elidedlabel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
