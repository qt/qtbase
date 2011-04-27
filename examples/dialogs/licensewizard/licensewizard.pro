HEADERS       = licensewizard.h
SOURCES       = licensewizard.cpp \
                main.cpp
RESOURCES     = licensewizard.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/licensewizard
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/licensewizard
INSTALLS += target sources

symbian: CONFIG += qt_example
