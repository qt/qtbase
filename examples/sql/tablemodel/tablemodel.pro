HEADERS       = ../connection.h
SOURCES       = tablemodel.cpp
QT           += sql widgets
requires(qtConfig(tableview))

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/tablemodel
INSTALLS += target

