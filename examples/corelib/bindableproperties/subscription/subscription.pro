QT += widgets
TARGET = subscription

SOURCES += main.cpp \
    subscription.cpp \
    user.cpp \
    ../shared/subscriptionwindow.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/corelib/bindableproperties/subscription
INSTALLS += target

FORMS += \
    ../shared/subscriptionwindow.ui

HEADERS += \
    subscription.h \
    user.h \
    ../shared/subscriptionwindow.h

RESOURCES += \
    ../shared/countries.qrc

