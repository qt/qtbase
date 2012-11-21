QT -= gui

CONFIG += console
CONFIG -= app_bundle  
TEMPLATE = app
SOURCES += hellothreadpool.cpp 
   
# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/threads/hellothreadpool
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS hellothreadpool.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/threads/hellothreadpool
INSTALLS += target sources




