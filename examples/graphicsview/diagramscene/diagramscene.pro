HEADERS	    =   mainwindow.h \
		diagramitem.h \
		diagramscene.h \
		arrow.h \
		diagramtextitem.h
SOURCES	    =   mainwindow.cpp \
		diagramitem.cpp \
		main.cpp \
		arrow.cpp \
		diagramtextitem.cpp \
		diagramscene.cpp
RESOURCES   =	diagramscene.qrc


# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/diagramscene
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS diagramscene.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/diagramscene
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

QT += widgets
symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
