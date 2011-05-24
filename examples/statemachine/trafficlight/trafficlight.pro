SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/trafficlight
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS trafficlight.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/trafficlight
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

