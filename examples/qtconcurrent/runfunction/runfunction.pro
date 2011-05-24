TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += main.cpp
CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent/runfunction
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent/runfunction
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

simulator: warning(This example does not work on Simulator platform)
