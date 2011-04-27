#! [0]
TEMPLATE    = subdirs
SUBDIRS	    = echowindow \
	      plugin
#! [0]

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/echoplugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS echoplugin.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/echoplugin
INSTALLS += target sources

symbian: CONFIG += qt_example
