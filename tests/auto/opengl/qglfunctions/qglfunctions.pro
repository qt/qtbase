load(qttest_p4)
requires(contains(QT_CONFIG,opengl))
QT += opengl widgets

win32:!wince*: DEFINES += QT_NO_EGL

SOURCES += tst_qglfunctions.cpp
