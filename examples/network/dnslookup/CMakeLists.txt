# Generated from dnslookup.pro.

cmake_minimum_required(VERSION 3.14)
project(dnslookup LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/network/dnslookup")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Network)

qt_add_executable(dnslookup
    dnslookup.cpp dnslookup.h
)
set_target_properties(dnslookup PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)
target_link_libraries(dnslookup PUBLIC
    Qt::Core
    Qt::Network
)

install(TARGETS dnslookup
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
