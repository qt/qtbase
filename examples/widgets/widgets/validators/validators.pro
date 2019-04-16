QT += widgets
requires(qtConfig(combobox))

FORMS += validators.ui
RESOURCES += validators.qrc

SOURCES += main.cpp ledwidget.cpp localeselector.cpp validatorwidget.cpp
HEADERS += ledwidget.h localeselector.h validatorwidget.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/validators
INSTALLS += target
