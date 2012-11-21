SOURCES   = addressbook.cpp \
            finddialog.cpp \
            main.cpp
HEADERS   = addressbook.h \
            finddialog.h

QMAKE_PROJECT_NAME = ab_part6

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook/part6
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part6.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/addressbook/part6
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
