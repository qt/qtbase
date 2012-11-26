SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine/eventtransitions
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS eventtransitions.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine/eventtransitions
INSTALLS += target sources
QT += widgets


simulator: warning(This example might not fully work on Simulator platform)
