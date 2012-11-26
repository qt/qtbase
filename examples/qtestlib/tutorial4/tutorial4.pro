SOURCES = testgui.cpp
QT += testlib

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtestlib/tutorial4
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtestlib/tutorial4
INSTALLS += target sources

QT += widgets
