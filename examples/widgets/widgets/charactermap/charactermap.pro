HEADERS     = characterwidget.h \
              mainwindow.h
SOURCES     = characterwidget.cpp \
              mainwindow.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/charactermap
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS charactermap.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/charactermap
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
