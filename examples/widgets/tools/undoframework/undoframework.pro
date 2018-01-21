QT += widgets
requires(qtConfig(undoview))

HEADERS	    = commands.h \
	      diagramitem.h \
	      diagramscene.h \
	      mainwindow.h
SOURCES	    = commands.cpp \
              diagramitem.cpp \
              diagramscene.cpp \
              main.cpp \
              mainwindow.cpp
RESOURCES   = undoframework.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/undoframework
INSTALLS += target
