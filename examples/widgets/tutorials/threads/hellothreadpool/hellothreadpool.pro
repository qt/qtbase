QT -= gui

CONFIG += console
CONFIG -= app_bundle  
TEMPLATE = app
SOURCES += hellothreadpool.cpp 
   
# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/threads/hellothreadpool
INSTALLS += target




