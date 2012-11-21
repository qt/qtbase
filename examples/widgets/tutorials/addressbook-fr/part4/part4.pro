SOURCES = addressbook.cpp \
          main.cpp
HEADERS = addressbook.h

QMAKE_PROJECT_NAME = abfr_part4

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook-fr/part4
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part4.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook-fr/part4
INSTALLS += target sources
QT += widgets


simulator: warning(This example might not fully work on Simulator platform)
