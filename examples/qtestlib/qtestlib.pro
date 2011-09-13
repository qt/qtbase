TEMPLATE = subdirs
SUBDIRS = tutorial1 tutorial2 tutorial3 tutorial4 tutorial5

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtestlib
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS qtestlib.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtestlib
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example
