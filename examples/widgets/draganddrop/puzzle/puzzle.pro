HEADERS     = mainwindow.h \
              pieceslist.h \
              puzzlewidget.h
RESOURCES   = puzzle.qrc
SOURCES     = main.cpp \
              mainwindow.cpp \
              pieceslist.cpp \
              puzzlewidget.cpp

QMAKE_PROJECT_NAME = dndpuzzle

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/draganddrop/puzzle
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.jpg
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/draganddrop/puzzle
INSTALLS += target sources

wince*: {
   addFile.files = example.jpg
   addFile.path = .
   DEPLOYMENT += addFile
}
QT += widgets
