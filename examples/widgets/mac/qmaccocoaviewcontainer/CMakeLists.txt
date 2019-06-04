# Generated from qmaccocoaviewcontainer.pro.

cmake_minimum_required(VERSION 3.14)
project(qmaccocoaviewcontainer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(qmaccocoaviewcontainer WIN32 MACOSX_BUNDLE
    main.mm
)
target_link_libraries(qmaccocoaviewcontainer PUBLIC
    ${FWAppKit}
    Qt::Widgets
)

install(TARGETS qmaccocoaviewcontainer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
