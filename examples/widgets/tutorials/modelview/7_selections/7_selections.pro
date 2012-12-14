TARGET = mv_selections
TEMPLATE = app
QT += widgets
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h 

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/modelview/7_selections
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
