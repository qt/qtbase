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
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/diagramscene
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS diagramscene.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/diagramscene
INSTALLS += target sources


QT += widgets
simulator: warning(This example might not fully work on Simulator platform)
