HEADERS	    =	mainwindow.h \
		tabletcanvas.h \
		tabletapplication.h
SOURCES	    =	mainwindow.cpp \
		main.cpp \
		tabletcanvas.cpp \
		tabletapplication.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/tablet
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS tablet.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/tablet
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
