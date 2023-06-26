QT += widgets
requires(qtConfig(listwidget))
qtHaveModule(printsupport): QT += printsupport

HEADERS         = mainwindow.h
SOURCES         = main.cpp \
                  mainwindow.cpp
RESOURCES       = dockwidgets.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mainwindows/dockwidgets
INSTALLS += target
