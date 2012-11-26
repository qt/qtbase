SOURCES   = addressbook.cpp \
            main.cpp
HEADERS   = addressbook.h

QMAKE_PROJECT_NAME = abfr_part2

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook-fr/part2
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part2.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook-fr/part2
INSTALLS += target sources
QT += widgets


simulator: warning(This example might not fully work on Simulator platform)
