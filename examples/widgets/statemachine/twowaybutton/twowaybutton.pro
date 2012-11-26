SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine/twowaybutton
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS twowaybutton.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine/twowaybutton
INSTALLS += target sources
QT += widgets


