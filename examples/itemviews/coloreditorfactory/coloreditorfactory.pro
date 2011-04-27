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

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
