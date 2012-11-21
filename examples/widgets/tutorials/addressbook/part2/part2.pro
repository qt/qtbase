SOURCES   = addressbook.cpp \
            main.cpp
HEADERS   = addressbook.h

QMAKE_PROJECT_NAME = ab_part2

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook/part2
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part2.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook/part2
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
