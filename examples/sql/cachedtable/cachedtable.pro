HEADERS       = ../connection.h \
                tableeditor.h
SOURCES       = main.cpp \
                tableeditor.cpp
QT           += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/cachedtable
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS cachedtable.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/sql/cachedtable
INSTALLS += target sources

