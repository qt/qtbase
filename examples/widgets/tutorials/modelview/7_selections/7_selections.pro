TARGET = mv_selections
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp
HEADERS += mainwindow.h 

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/7_selections
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 7_selections.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/modelview/7_selections
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
