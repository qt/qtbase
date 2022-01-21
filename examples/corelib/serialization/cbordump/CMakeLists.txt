cmake_minimum_required(VERSION 3.16)
project(cbordump LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/serialization/cbordump")

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(cbordump
    main.cpp
)

target_link_libraries(cbordump PUBLIC
    Qt::Core
)

install(TARGETS cbordump
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
