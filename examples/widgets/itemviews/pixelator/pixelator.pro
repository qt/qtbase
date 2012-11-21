HEADERS     = imagemodel.h \
              mainwindow.h \
              pixeldelegate.h
SOURCES     = imagemodel.cpp \
              main.cpp \
              mainwindow.cpp \
              pixeldelegate.cpp
RESOURCES   += images.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/pixelator
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/itemviews/pixelator
INSTALLS += target sources

QT += widgets
!isEmpty(QT.printsupport.name): QT += printsupport
