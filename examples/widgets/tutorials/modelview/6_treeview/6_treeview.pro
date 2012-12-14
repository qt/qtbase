TARGET = mv_tree
TEMPLATE = app
QT += widgets
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/6_treeview
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
