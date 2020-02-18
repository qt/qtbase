qtHaveModule(windowsuiautomation_support-private): \
    QT += windowsuiautomation_support-private

SOURCES += \
    $$PWD/qwindowsuiaaccessibility.cpp \
    $$PWD/qwindowsuiaprovidercache.cpp \
    $$PWD/qwindowsuiamainprovider.cpp \
    $$PWD/qwindowsuiabaseprovider.cpp \
    $$PWD/qwindowsuiavalueprovider.cpp \
    $$PWD/qwindowsuiatextprovider.cpp \
    $$PWD/qwindowsuiatextrangeprovider.cpp \
    $$PWD/qwindowsuiatoggleprovider.cpp \
    $$PWD/qwindowsuiaselectionprovider.cpp \
    $$PWD/qwindowsuiaselectionitemprovider.cpp \
    $$PWD/qwindowsuiainvokeprovider.cpp \
    $$PWD/qwindowsuiarangevalueprovider.cpp \
    $$PWD/qwindowsuiatableprovider.cpp \
    $$PWD/qwindowsuiatableitemprovider.cpp \
    $$PWD/qwindowsuiagridprovider.cpp \
    $$PWD/qwindowsuiagriditemprovider.cpp \
    $$PWD/qwindowsuiawindowprovider.cpp \
    $$PWD/qwindowsuiaexpandcollapseprovider.cpp \
    $$PWD/qwindowsuiautils.cpp

HEADERS += \
    $$PWD/qwindowsuiaaccessibility.h \
    $$PWD/qwindowsuiaprovidercache.h \
    $$PWD/qwindowsuiamainprovider.h \
    $$PWD/qwindowsuiabaseprovider.h \
    $$PWD/qwindowsuiavalueprovider.h \
    $$PWD/qwindowsuiatextprovider.h \
    $$PWD/qwindowsuiatextrangeprovider.h \
    $$PWD/qwindowsuiatoggleprovider.h \
    $$PWD/qwindowsuiaselectionprovider.h \
    $$PWD/qwindowsuiaselectionitemprovider.h \
    $$PWD/qwindowsuiainvokeprovider.h \
    $$PWD/qwindowsuiarangevalueprovider.h \
    $$PWD/qwindowsuiatableprovider.h \
    $$PWD/qwindowsuiatableitemprovider.h \
    $$PWD/qwindowsuiagridprovider.h \
    $$PWD/qwindowsuiagriditemprovider.h \
    $$PWD/qwindowsuiawindowprovider.h \
    $$PWD/qwindowsuiaexpandcollapseprovider.h \
    $$PWD/qwindowsuiautils.h

mingw: QMAKE_USE *= uuid
