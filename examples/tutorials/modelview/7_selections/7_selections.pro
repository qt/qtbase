TARGET = mv_selections
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h 

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/7_selections
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 7_selections.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/7_selections
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
