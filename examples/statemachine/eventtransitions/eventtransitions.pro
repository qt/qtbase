SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/eventtransitions
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS eventtransitions.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/eventtransitions
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

simulator: warning(This example might not fully work on Simulator platform)
