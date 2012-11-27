HEADERS   = window.h
SOURCES   = main.cpp \
            window.cpp
QT += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/sqlwidgetmapper
INSTALLS += target

wince*: DEPLOYMENT_PLUGIN += qsqlite


