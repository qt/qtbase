# Generated from controller.pro.

cmake_minimum_required(VERSION 3.14)
project(controller LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS DBus)
find_package(Qt6 COMPONENTS Widgets)

# special case begin
set(controller_SRCS)
qt6_add_dbus_interface(controller_SRCS
    car.xml
    car_interface
)
# special case end
add_qt_gui_executable(controller
    controller.cpp controller.h controller.ui
    main.cpp
    ${controller_SRCS} # special case
)
target_link_libraries(controller PUBLIC
    Qt::DBus
    Qt::Widgets
)
install(TARGETS controller
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
