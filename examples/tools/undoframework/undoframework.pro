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
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/undoframework
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS undoframework.pro README images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/undoframework
INSTALLS += target sources

symbian: CONFIG += qt_example
