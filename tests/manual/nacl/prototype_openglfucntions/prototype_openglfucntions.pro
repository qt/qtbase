TEMPLATE = app
TARGET = opengl_functions
DEPENDPATH += .

SOURCES += main.cpp
QT -= network core gui

LIBS +=  -lppapi -lnacl_io -lppapi_gles2