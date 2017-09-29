cmake_minimum_required(VERSION 2.8)
project(test_dependent_modules)

find_package(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# make sure CMP0071 warnings cause a test failure
set(CMAKE_SUPPRESS_DEVELOPER_ERRORS FALSE CACHE INTERNAL "" FORCE)

qt5_wrap_cpp(moc_files mywidget.h)
qt5_wrap_ui(ui_files mywidget.ui)
qt5_add_resources(qrc_files res.qrc)

add_executable(mywidget
    # source files
    mywidget.cpp
    mywidget.h
    mywidget.ui
    res.qrc

    # generated files
    ${moc_files}
    ${ui_files}
    ${qrc_files}
)
target_link_libraries(mywidget ${Qt5Widgets_LIBRARIES})
