TARGET = mv_tree
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/6_treeview
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
