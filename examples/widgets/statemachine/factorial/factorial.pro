QT = core
win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/statemachine/factorial
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS factorial.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/statemachine/factorial
INSTALLS += target sources


