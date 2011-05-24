QT = core
win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/factorial
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS factorial.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/factorial
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example does not work on Symbian platform)
