TEMPLATE = app

TARGET = buttontester
TEMPLATE = app

SOURCES += \
    main.cpp\
    buttontester.cpp \

HEADERS += \
    buttontester.h \

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/mousebuttons
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS mousebuttons.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/mousebuttons
INSTALLS += target sources
QT += core widgets
