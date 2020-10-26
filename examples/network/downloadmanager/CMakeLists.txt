# Generated from downloadmanager.pro.

cmake_minimum_required(VERSION 3.14)
project(downloadmanager LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/network/downloadmanager")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Network)

qt_add_executable(downloadmanager
    downloadmanager.cpp downloadmanager.h
    main.cpp
    textprogressbar.cpp textprogressbar.h
)
set_target_properties(downloadmanager PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)
target_link_libraries(downloadmanager PUBLIC
    Qt::Core
    Qt::Network
)

install(TARGETS downloadmanager
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
