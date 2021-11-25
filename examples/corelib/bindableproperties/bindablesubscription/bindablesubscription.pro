QT += widgets
TARGET = bindablesubscription

SOURCES += main.cpp \
    bindablesubscription.cpp \
    bindableuser.cpp \
    ../shared/subscriptionwindow.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/corelib/bindableproperties/bindablesubscription
INSTALLS += target

FORMS += \
    ../shared/subscriptionwindow.ui

HEADERS += \
    bindablesubscription.h \
    bindableuser.h \
    ../shared/subscriptionwindow.h

RESOURCES += \
    ../shared/countries.qrc

