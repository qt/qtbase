TEMPLATE      = subdirs
SUBDIRS       = \
                factorial \
                pingpong

!contains(QT_CONFIG, no-widgets) {
    SUBDIRS +=  \
                eventtransitions \
                rogue \
                trafficlight \
                twowaybutton
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS statemachine.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine
INSTALLS += target sources
