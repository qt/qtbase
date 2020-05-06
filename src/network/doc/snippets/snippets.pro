TEMPLATE = app

TARGET = network_cppsnippets

# ![0]
QT += network
# ![0]

SOURCES += network/tcpwait.cpp

load(qt_common)
