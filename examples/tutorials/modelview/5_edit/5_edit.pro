TARGET = mv_edit

TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           mymodel.cpp

HEADERS += mainwindow.h \
           mymodel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/5_edit
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS 5_edit.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/modelview/5_edit
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
