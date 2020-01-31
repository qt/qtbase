# Generated from sub-attaq.pro.

cmake_minimum_required(VERSION 3.14)
project(sub-attaq LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS OpenGL) # special case

add_qt_gui_executable(sub-attaq
    animationmanager.cpp animationmanager.h
    boat.cpp boat.h boat_p.h
    bomb.cpp bomb.h
    graphicsscene.cpp graphicsscene.h
    main.cpp
    mainwindow.cpp mainwindow.h
    pixmapitem.cpp pixmapitem.h
    progressitem.cpp progressitem.h
    qanimationstate.cpp qanimationstate.h
    states.cpp states.h
    subattaq.qrc
    submarine.cpp submarine.h submarine_p.h
    textinformationitem.cpp textinformationitem.h
    torpedo.cpp torpedo.h
)
target_link_libraries(sub-attaq PUBLIC
    Qt::Widgets
)
if (TARGET Qt::OpenGL)
    target_link_libraries(sub-attaq PUBLIC Qt::OpenGL)
endif()

install(TARGETS sub-attaq
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
