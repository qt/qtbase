HEADERS += rsslisting.h
SOURCES += main.cpp rsslisting.cpp
QT += network xml widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/rsslisting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS rsslisting.pro 
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/rsslisting
INSTALLS += target sources

symbian {
    CONFIG += qt_example

    # For QtMobility
    CONFIG += mobility
    MOBILITY = bearer

    # For QtMobility
    TARGET.CAPABILITY = NetworkServices
}

maemo5: CONFIG += qt_example
