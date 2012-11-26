HEADERS       = ../connection.h
SOURCES       = tablemodel.cpp
QT           += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/tablemodel
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS tablemodel.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/sql/tablemodel
INSTALLS += target sources

