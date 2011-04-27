TEMPLATE	= subdirs
SUBDIRS		= stylewindow \
		  plugin

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS styleplugin.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
