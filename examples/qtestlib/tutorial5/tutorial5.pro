SOURCES = benchmarking.cpp
QT += testlib

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtestlib/tutorial5
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtestlib/tutorial5
INSTALLS += target sources

QT += widgets
