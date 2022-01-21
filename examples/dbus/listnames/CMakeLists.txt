cmake_minimum_required(VERSION 3.16)
project(listnames LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/dbus/listnames")

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

qt_add_executable(listnames
    listnames.cpp
)

target_link_libraries(listnames PUBLIC
    Qt::Core
    Qt::DBus
)

install(TARGETS listnames
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
