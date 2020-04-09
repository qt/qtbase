requires(qtConfig(private_tests))
TEMPLATE = app
QT = network-private testlib

SOURCES = main.cpp
CONFIG += release

DEFINES += SRC_DIR="$$PWD"
