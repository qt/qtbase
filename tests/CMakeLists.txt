# special case begin
if(NOT TARGET Qt::Test)
    cmake_minimum_required(VERSION 3.14.0)
    project(QtBaseTests C CXX ASM)
    find_package(Qt5 REQUIRED COMPONENTS BuildInternals Core Test)
    find_package(Qt5 COMPONENTS DBus Gui OpenGL Widgets Xml)
    qt_set_up_standalone_tests_build()

    # Find a few packages that are usually found in configure.cmake,
    # because a few tests link directly against those libraries.
    qt_find_package(WrapDBus1 PROVIDED_TARGETS dbus-1)
    qt_find_package(WrapOpenGL PROVIDED_TARGETS WrapOpenGL)
endif()
# special case end

qt_build_tests()
