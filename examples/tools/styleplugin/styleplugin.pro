TEMPLATE	= subdirs
SUBDIRS		= stylewindow \
		  plugin

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS styleplugin.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/styleplugin
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example
