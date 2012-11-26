HEADERS       = ../connection.h \
                customsqlmodel.h \
                editablesqlmodel.h
SOURCES       = customsqlmodel.cpp \
                editablesqlmodel.cpp \
                main.cpp
QT           += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/querymodel
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS querymodel.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/sql/querymodel
INSTALLS += target sources

