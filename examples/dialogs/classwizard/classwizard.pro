HEADERS       = classwizard.h
SOURCES       = classwizard.cpp \
                main.cpp
RESOURCES     = classwizard.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/classwizard
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/dialogs/classwizard
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
