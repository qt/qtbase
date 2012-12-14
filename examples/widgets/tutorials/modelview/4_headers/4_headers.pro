TARGET = mv_headers

TEMPLATE = app

QT += widgets

SOURCES += main.cpp  \
           mymodel.cpp

HEADERS += mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/4_headers
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
