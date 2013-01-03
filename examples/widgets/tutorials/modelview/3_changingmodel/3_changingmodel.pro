TARGET = mv_changingmodel

TEMPLATE = app

QT += widgets

SOURCES += main.cpp \
           mymodel.cpp

HEADERS += mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/3_changingmodel
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
