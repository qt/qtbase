
# Cause make to do nothing.
TEMPLATE = subdirs

CMAKE_QT_MODULES_UNDER_TEST = core network xml sql testlib

qtHaveModule(dbus): CMAKE_QT_MODULES_UNDER_TEST += dbus
qtHaveModule(gui): CMAKE_QT_MODULES_UNDER_TEST += gui
qtHaveModule(openglextensions): CMAKE_QT_MODULES_UNDER_TEST += openglextensions
qtHaveModule(widgets): CMAKE_QT_MODULES_UNDER_TEST += widgets
qtHaveModule(printsupport): CMAKE_QT_MODULES_UNDER_TEST += printsupport
qtHaveModule(opengl): CMAKE_QT_MODULES_UNDER_TEST += opengl
qtHaveModule(concurrent): CMAKE_QT_MODULES_UNDER_TEST += concurrent

CONFIG += ctest_testcase
win32:testcase.timeout = 1000 # this test is slow on Windows
