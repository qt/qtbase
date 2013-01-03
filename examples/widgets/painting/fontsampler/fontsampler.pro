QT += widgets
qtHaveModule(printsupport): QT += printsupport

FORMS     = mainwindowbase.ui
HEADERS   = mainwindow.h
SOURCES   = main.cpp \
            mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/fontsampler
INSTALLS += target
