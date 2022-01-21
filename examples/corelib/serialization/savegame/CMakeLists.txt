cmake_minimum_required(VERSION 3.16)
project(savegame LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/corelib/serialization/savegame")

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(savegame
    character.cpp character.h
    game.cpp game.h
    level.cpp level.h
    main.cpp
)

target_link_libraries(savegame PUBLIC
    Qt::Core
)

install(TARGETS savegame
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
