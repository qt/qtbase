HEADERS       = ../connection.h
SOURCES       = relationaltablemodel.cpp
QT           += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/relationaltablemodel
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS relationaltablemodel.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/sql/relationaltablemodel
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
