TEMPLATE = app

HEADERS += collector.h
SOURCES += main.cpp collector.cpp
LIBS += -lscreen

QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/qnx/foreignwindows
INSTALLS += target
