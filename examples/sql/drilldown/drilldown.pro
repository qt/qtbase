HEADERS = ../connection.h \
          imageitem.h \
          informationwindow.h \
          view.h
RESOURCES = drilldown.qrc
SOURCES = imageitem.cpp \
          informationwindow.cpp \
          main.cpp \
          view.cpp
QT += sql widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/sql/drilldown
INSTALLS += target
