HEADERS	    = colorlisteditor.h \
	      window.h
SOURCES	    = colorlisteditor.cpp \
	      window.cpp \
	      main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/coloreditorfactory
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/coloreditorfactory
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
