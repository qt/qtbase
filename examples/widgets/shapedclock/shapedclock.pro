HEADERS       = shapedclock.h
SOURCES       = shapedclock.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/shapedclock
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS shapedclock.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/shapedclock
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C605
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
