TEMPLATE = app
TARGET = BearerEx

QT += core \
      gui \
      widgets \
      network

FORMS += detailedinfodialog.ui \
         sessiondialog.ui \
         bearerex.ui

# Example headers and sources
HEADERS += bearerex.h \
           xqlistwidget.h \
    datatransferer.h
    
SOURCES += bearerex.cpp \
           main.cpp \
           xqlistwidget.cpp \
    datatransferer.cpp
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
