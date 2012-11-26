TEMPLATE	= subdirs
SUBDIRS		= stylewindow \
		  plugin

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/styleplugin
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS styleplugin.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tools/styleplugin
INSTALLS += target sources

QT += widgets
