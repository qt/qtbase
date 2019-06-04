# Generated from car.pro.

cmake_minimum_required(VERSION 3.14)
project(car LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS DBus)
find_package(Qt5 COMPONENTS Widgets)

# special case begin
set(car_SRCS)
qt5_add_dbus_adaptor(car_SRCS
    car.xml
    car.h
    Car
    car_adaptor
)
# special case end
add_executable(car WIN32 MACOSX_BUNDLE
    car.cpp car.h
    main.cpp
    ${car_SRCS} # special case
)
target_link_libraries(car PUBLIC
    Qt::DBus
    Qt::Widgets
)
install(TARGETS car
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
