SOURCES       = trivialwizard.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/trivialwizard
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/trivialwizard
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
