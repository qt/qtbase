SOURCES       = trivialwizard.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/trivialwizard
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/trivialwizard
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
symbian: warning(This example might not fully work on Symbian platform)
simulator: warning(This example might not fully work on Simulator platform)
