
cmake_minimum_required(VERSION 2.8)

project(test_opengl_lib)

find_package(Qt5Gui REQUIRED)

include_directories(${Qt5Gui_INCLUDE_DIRS} ${Qt5Gui_OPENGL_INCLUDE_DIRS} )
add_definitions(${Qt5Gui_DEFINITIONS})

get_property(QtGui_enabled_features TARGET Qt5::Gui PROPERTY INTERFACE_QT_ENABLED_FEATURES)
list(LENGTH QtGui_enabled_features _QtGui_enabled_featuresSize)
if (_QtGui_enabled_featuresSize LESS 1)
    message(SEND_ERROR "The INTERFACE_QT_ENABLED_FEATURES property of Qt5::Gui is empty!")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Gui_EXECUTABLE_COMPILE_FLAGS}")

if (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL GLESv2)
    list(FIND QtGui_enabled_features "opengles2" _opengles2Index)
    if(_opengles2Index EQUAL -1)
        message(SEND_ERROR "The INTERFACE_QT_ENABLED_FEATURES property of Qt5::Gui does not contain the opengles2 feature!")
    endif()
    add_definitions(-DGL_IMPLEMENTATION_GLES2)
elseif (${Qt5Gui_OPENGL_IMPLEMENTATION} STREQUAL GL)
    list(FIND QtGui_enabled_features "opengl" _openglIndex)
    if(_openglIndex EQUAL -1)
        message(SEND_ERROR "The INTERFACE_QT_ENABLED_FEATURES property of Qt5::Gui does not contain the opengl feature!")
    endif()
    add_definitions(-DGL_IMPLEMENTATION_GL)
else()
    message(SEND_ERROR "Qt5Gui_OPENGL_IMPLEMENTATION does not contain valid data.")
endif()

add_executable(myobject main.cpp )
target_link_libraries(myobject ${Qt5Gui_LIBRARIES} ${Qt5Gui_OPENGL_LIBRARIES})
