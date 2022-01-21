cmake_minimum_required(VERSION 3.16)
project(imagegestures LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/gestures/imagegestures")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(imagegestures
    imagewidget.cpp imagewidget.h
    main.cpp
    mainwidget.cpp mainwidget.h
)

set_target_properties(imagegestures PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(imagegestures PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS imagegestures
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
