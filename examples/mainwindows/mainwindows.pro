TEMPLATE      = subdirs
SUBDIRS       = application \
                mdi \
                menus \
                recentfiles \
                sdi

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS mainwindows.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
