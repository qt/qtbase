# Nokia Qt Examples: elided label example

QT += core gui widgets

TARGET = elidedlabel
TEMPLATE = app

SOURCES += \
    main.cpp\
    testwidget.cpp \
    elidedlabel.cpp

HEADERS += \
    testwidget.h \
    elidedlabel.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/elidedlabel
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/elidedlabel
INSTALLS += target sources
