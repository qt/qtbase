HEADERS     = mainwindow.h
SOURCES     = main.cpp \
              mainwindow.cpp

# App cannot be with name "calendar" in Symbian due to same named system component.

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/richtext/calendar
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS calendar.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/richtext/calendar
INSTALLS += target sources

QT += widgets
