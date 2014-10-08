TEMPLATE = app

TARGET = mousebuttons
TEMPLATE = app

SOURCES += \
    main.cpp\
    buttontester.cpp \

HEADERS += \
    buttontester.h \

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/mousebuttons
INSTALLS += target
QT += core widgets
