TEMPLATE = app
TARGET = flightinfo
SOURCES = flightinfo.cpp
FORMS += form.ui
RESOURCES = flightinfo.qrc
QT += network

symbian {
    TARGET.UID3 = 0xA000CF74
    CONFIG += qt_demo
    TARGET.CAPABILITY = NetworkServices
}

target.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/flightinfo
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_DEMOS]/qtbase/embedded/flightinfo
INSTALLS += target sources
