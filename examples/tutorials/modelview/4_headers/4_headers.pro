TARGET = mv_headers

TEMPLATE = app

SOURCES += main.cpp  \
           mymodel.cpp

HEADERS += mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/4_headers
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 4_headers.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/4_headers
INSTALLS += target sources

symbian: CONFIG += qt_example
