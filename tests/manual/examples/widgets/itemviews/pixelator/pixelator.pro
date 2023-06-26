QT += widgets
requires(qtConfig(tableview))
qtHaveModule(printsupport): QT += printsupport

HEADERS     = imagemodel.h \
              mainwindow.h \
              pixeldelegate.h
SOURCES     = imagemodel.cpp \
              main.cpp \
              mainwindow.cpp \
              pixeldelegate.cpp
RESOURCES   += images.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/pixelator
INSTALLS += target
