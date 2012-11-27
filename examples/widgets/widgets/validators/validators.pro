QT += widgets

FORMS += validators.ui
RESOURCES += validators.qrc

SOURCES += main.cpp ledwidget.cpp localeselector.cpp
HEADERS += ledwidget.h localeselector.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/validators
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
