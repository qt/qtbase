SOURCES += main.cpp
RESOURCES += states.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation/states
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS states.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation/states
INSTALLS += target sources

QT += widgets
