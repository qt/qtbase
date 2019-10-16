# special case begin
if(NOT TARGET Qt::Test)
    cmake_minimum_required(VERSION 3.14.0)
    project(QtBaseTests VERSION 6.0.0 LANGUAGES C CXX ASM)
    find_package(Qt6 REQUIRED COMPONENTS BuildInternals Core Test)
    find_package(Qt6 COMPONENTS DBus Gui OpenGL Widgets Xml)
    qt_set_up_standalone_tests_build()

    # Find a few packages that are usually found in configure.cmake,
    # because a few tests link directly against those libraries.
    qt_find_package(WrapDBus1 PROVIDED_TARGETS dbus-1)
    qt_find_package(ICU COMPONENTS i18n uc data PROVIDED_TARGETS ICU::i18n ICU::uc ICU::data)
    qt_find_package(Threads PROVIDED_TARGETS Threads::Threads)
endif()
# special case end

qt_build_tests()
