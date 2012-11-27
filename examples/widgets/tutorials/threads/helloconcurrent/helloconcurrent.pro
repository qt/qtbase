QT -= gui
QT += concurrent

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += helloconcurrent.cpp 
   
# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/threads/helloconcurrent
INSTALLS += target



