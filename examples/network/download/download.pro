QT = core network
CONFIG += console
CONFIG -= app_bundle

SOURCES += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/download
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/network/download
INSTALLS += target sources

simulator: warning(This example might not fully work on Simulator platform)
