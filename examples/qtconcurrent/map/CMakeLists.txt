cmake_minimum_required(VERSION 3.16)
project(mapdemo LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/qtconcurrent/map")

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui)

qt_add_executable(mapdemo
    main.cpp
)

target_link_libraries(mapdemo PUBLIC
    Qt::Concurrent
    Qt::Core
    Qt::Gui
)

install(TARGETS mapdemo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
