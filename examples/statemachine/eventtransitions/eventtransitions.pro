SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/eventtransitions
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS eventtransitions.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/eventtransitions
INSTALLS += target sources
