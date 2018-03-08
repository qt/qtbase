# Nokia Qt Examples: elided label example

QT += core gui widgets
requires(qtConfig(combobox))

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
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/elidedlabel
INSTALLS += target
