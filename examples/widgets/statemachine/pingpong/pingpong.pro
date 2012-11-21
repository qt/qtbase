QT = core
win32: CONFIG += console
mac:CONFIG -= app_bundle

SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/statemachine/pingpong
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS pingpong.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/statemachine/pingpong
INSTALLS += target sources


