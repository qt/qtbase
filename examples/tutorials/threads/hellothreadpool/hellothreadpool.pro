QT -= gui

CONFIG += console
CONFIG -= app_bundle  
TEMPLATE = app
SOURCES += hellothreadpool.cpp 
   
# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/threads/hellothreadpool
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS hellothreadpool.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/threads/hellothreadpool
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)



