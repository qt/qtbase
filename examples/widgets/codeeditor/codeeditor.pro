HEADERS     = codeeditor.h
SOURCES     = main.cpp \
              codeeditor.cpp
# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/codeeditor
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/codeeditor
INSTALLS += target sources

QT += widgets
symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
