QT += widgets

HEADERS	    =	mainwindow.h \
		tabletcanvas.h \
		tabletapplication.h
SOURCES	    =	mainwindow.cpp \
		main.cpp \
		tabletcanvas.cpp \
		tabletapplication.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/tablet
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
