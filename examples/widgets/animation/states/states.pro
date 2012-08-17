SOURCES += main.cpp
RESOURCES += states.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/states
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS states.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/states
INSTALLS += target sources

QT += widgets
