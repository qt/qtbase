HEADERS   = window.h
SOURCES   = main.cpp \
            window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/combowidgetmapper
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/combowidgetmapper
INSTALLS += target sources
QT += widgets

