HEADERS       = shapedclock.h
SOURCES       = shapedclock.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/shapedclock
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS shapedclock.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/shapedclock
INSTALLS += target sources

QT += widgets

