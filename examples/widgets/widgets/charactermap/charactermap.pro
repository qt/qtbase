HEADERS     = characterwidget.h \
              mainwindow.h
SOURCES     = characterwidget.cpp \
              mainwindow.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/charactermap
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
