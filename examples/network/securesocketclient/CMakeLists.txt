# Generated from securesocketclient.pro.

cmake_minimum_required(VERSION 3.14)
project(securesocketclient LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(securesocketclient
    certificateinfo.cpp certificateinfo.h certificateinfo.ui
    main.cpp
    securesocketclient.qrc
    sslclient.cpp sslclient.h sslclient.ui
    sslerrors.ui
)
target_link_libraries(securesocketclient PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS securesocketclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
