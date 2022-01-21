cmake_minimum_required(VERSION 3.16)
project(waitconditions LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/threads/waitconditions")

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(waitconditions
    waitconditions.cpp
)

target_link_libraries(waitconditions PUBLIC
    Qt::Core
)

install(TARGETS waitconditions
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
