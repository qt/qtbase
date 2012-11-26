HEADERS       = ../connection.h
SOURCES       = relationaltablemodel.cpp
QT           += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/relationaltablemodel
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS relationaltablemodel.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/sql/relationaltablemodel
INSTALLS += target sources

