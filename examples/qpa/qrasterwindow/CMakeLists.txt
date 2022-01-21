cmake_minimum_required(VERSION 3.16)
project(qrasterwindow LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qpa/qrasterwindow")

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

qt_add_executable(qrasterwindow
    main.cpp
)

set_target_properties(qrasterwindow PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(qrasterwindow PUBLIC
    Qt::Core
    Qt::Gui
)

install(TARGETS qrasterwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
