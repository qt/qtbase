cmake_minimum_required(VERSION 3.16)
project(downloadmanager LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/network/downloadmanager")

find_package(Qt6 REQUIRED COMPONENTS Core Network)

qt_add_executable(downloadmanager
    downloadmanager.cpp downloadmanager.h
    main.cpp
    textprogressbar.cpp textprogressbar.h
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
