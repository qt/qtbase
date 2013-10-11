QT = core network
CONFIG += console
CONFIG -= app_bundle

HEADERS += downloadmanager.h textprogressbar.h
SOURCES += downloadmanager.cpp main.cpp textprogressbar.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/downloadmanager
INSTALLS += target

OTHER_FILES += \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/README \
    debian/rules
