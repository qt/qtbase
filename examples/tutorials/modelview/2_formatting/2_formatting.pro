TARGET = mv_formatting

TEMPLATE = app

SOURCES += main.cpp \
           mymodel.cpp

HEADERS += mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/2_formatting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 2_formatting.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/2_formatting
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
