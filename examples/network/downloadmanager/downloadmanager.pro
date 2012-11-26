QT = core network
CONFIG += console
CONFIG -= app_bundle

HEADERS += downloadmanager.h textprogressbar.h
SOURCES += downloadmanager.cpp main.cpp textprogressbar.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/downloadmanager
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/network/downloadmanager
INSTALLS += target sources

OTHER_FILES += \
    debian/changelog \
    debian/compat \
    debian/control \
    debian/copyright \
    debian/README \
    debian/rules

simulator: warning(This example might not fully work on Simulator platform)
