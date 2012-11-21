TEMPLATE      = subdirs
SUBDIRS       = application \
		dockwidgets \
                mainwindow \
                mdi \
                menus \
                recentfiles \
                sdi

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS mainwindows.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows
INSTALLS += target sources

QT += widgets
