TEMPLATE	= subdirs
SUBDIRS		= stylewindow \
		  plugin

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/styleplugin
INSTALLS += target
