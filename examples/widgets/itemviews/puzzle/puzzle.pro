QT += widgets

HEADERS     = mainwindow.h \
              piecesmodel.h \
              puzzlewidget.h
RESOURCES   = puzzle.qrc
SOURCES     = main.cpp \
              mainwindow.cpp \
              piecesmodel.cpp \
              puzzlewidget.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/puzzle
INSTALLS += target


wince* {
   DEPLOYMENT_PLUGIN += qjpeg qgif
}
