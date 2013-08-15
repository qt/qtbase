
cmake_minimum_required(VERSION 2.8)

project(test_plugins)

find_package(Qt5Network 5.0.0 REQUIRED)

include_directories(
    ${Qt5Network_INCLUDE_DIRS}
)

add_definitions(${Qt5Netork_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Netork_EXECUTABLE_COMPILE_FLAGS}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT Qt5Network_PLUGINS)
  message(SEND_ERROR "Qt5 network plugins not known!")
endif()

foreach(plugin ${Qt5Network_PLUGINS})
  get_target_property(_loc ${plugin} LOCATION)
  if (NOT EXISTS "${_loc}")
    message(SEND_ERROR "Plugin ${plugin} not found at ${_loc}")
  endif()
endforeach()
