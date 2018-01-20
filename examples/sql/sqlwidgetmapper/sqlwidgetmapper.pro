HEADERS   = window.h
SOURCES   = main.cpp \
            window.cpp
QT += sql widgets
requires(qtConfig(combobox))

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/sqlwidgetmapper
INSTALLS += target


