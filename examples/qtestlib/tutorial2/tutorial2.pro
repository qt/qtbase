SOURCES = testqstring.cpp
QT += testlib

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtestlib/tutorial2
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtestlib/tutorial2
INSTALLS += target sources

QT += widgets
