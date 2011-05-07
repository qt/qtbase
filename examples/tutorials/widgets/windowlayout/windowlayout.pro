SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/widgets/windowlayout
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS windowlayout.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/widgets/windowlayout
INSTALLS += target sources
QT += widgets
