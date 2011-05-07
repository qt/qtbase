TEMPLATE      = subdirs
SUBDIRS       = application \
                dockwidgets \
                mdi \
                menus \
                recentfiles \
                sdi

symbian: SUBDIRS = \
                menus


# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS mainwindows.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
