SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/widgets/childwidget
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS childwidget.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/widgets/childwidget
INSTALLS += target sources
