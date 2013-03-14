
cmake_minimum_required(VERSION 2.8.3)

project(test_testlib_definitions)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

macro(test_testlib_project _module)
    find_package(Qt5${_module} REQUIRED)
    find_package(Qt5Test REQUIRED)

    add_definitions(
        ${Qt5${_module}_DEFINITIONS}
        ${Qt5Test_DEFINITIONS}
    )

    include_directories(
        ${Qt5${_module}_INCLUDE_DIRS}
        ${Qt5Test_INCLUDE_DIRS}
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5${_module}_EXECUTABLE_COMPILE_FLAGS}")

    set(main_file "${CMAKE_CURRENT_SOURCE_DIR}/../main.cpp")
    set(moc_file "${CMAKE_CURRENT_BINARY_DIR}/main.moc")

    qt5_generate_moc("${main_file}" "${moc_file}")

    add_executable(testapp_${_module} "${main_file}" "${moc_file}")
    target_link_libraries(testapp_${_module}
        ${Qt5${_module}_LIBRARIES}
        ${Qt5Test_LIBRARIES}
    )
endmacro()

add_subdirectory(core_only)
add_subdirectory(gui)
if(NOT NO_WIDGETS)
    add_subdirectory(widgets)
endif()
