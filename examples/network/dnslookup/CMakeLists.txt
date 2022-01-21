cmake_minimum_required(VERSION 3.16)
project(dnslookup LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/network/dnslookup")

find_package(Qt6 REQUIRED COMPONENTS Core Network)

qt_add_executable(dnslookup
    dnslookup.cpp dnslookup.h
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
