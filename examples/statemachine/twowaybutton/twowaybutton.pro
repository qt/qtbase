SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/twowaybutton
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS twowaybutton.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/twowaybutton
INSTALLS += target sources
QT += widgets

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

