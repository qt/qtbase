QT += concurrent widgets
CONFIG += console
CONFIG -= app_bundle

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/runfunction
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/runfunction
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
