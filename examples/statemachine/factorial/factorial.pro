QT = core
win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/factorial
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS factorial.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/statemachine/factorial
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example does not work on Symbian platform)
