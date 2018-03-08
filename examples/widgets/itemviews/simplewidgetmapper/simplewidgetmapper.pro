QT += widgets
requires(qtConfig(datawidgetmapper))

HEADERS   = window.h
SOURCES   = main.cpp \
            window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/simplewidgetmapper
INSTALLS += target
