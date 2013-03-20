
cmake_minimum_required(VERSION 2.8)

project(test_umbrella_config)

add_subdirectory(components_found)

if (Qt5_FOUND)
    message(SEND_ERROR "Qt5_FOUND variable leaked!")
endif()

if (Qt5Core_FOUND)
    message(SEND_ERROR "Qt5Core_FOUND variable leaked!")
endif()

if (TARGET Qt5::Core)
    message(SEND_ERROR "Qt5::Core target leaked!")
endif()

add_subdirectory(components_not_found)
