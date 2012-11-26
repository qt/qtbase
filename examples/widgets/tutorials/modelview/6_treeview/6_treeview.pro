TARGET = mv_tree
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/6_treeview
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 6_treeview.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/6_treeview
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
