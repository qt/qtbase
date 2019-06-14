# Generated from pixelator.pro.

cmake_minimum_required(VERSION 3.14)
project(pixelator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS PrintSupport) # special case

add_qt_gui_executable(pixelator
    imagemodel.cpp imagemodel.h
    images.qrc
    main.cpp
    mainwindow.cpp mainwindow.h
    pixeldelegate.cpp pixeldelegate.h
)
target_link_libraries(pixelator PUBLIC
    Qt::Widgets
)
if (TARGET Qt::PrintSupport)
    target_link_libraries(pixelator PUBLIC
        Qt::PrintSupport
    )
endif()

install(TARGETS pixelator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
