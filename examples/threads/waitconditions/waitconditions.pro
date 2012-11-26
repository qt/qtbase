QT = core gui
CONFIG -= moc app_bundle
CONFIG += console

SOURCES += waitconditions.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/threads/waitconditions
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS waitconditions.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/threads/waitconditions
INSTALLS += target sources

simulator: warning(This example might not fully work on Simulator platform)
