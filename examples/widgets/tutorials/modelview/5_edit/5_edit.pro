TARGET = mv_edit

TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           mymodel.cpp

HEADERS += mainwindow.h \
           mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/5_edit
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
