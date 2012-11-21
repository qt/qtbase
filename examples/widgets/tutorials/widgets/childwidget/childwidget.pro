SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/widgets/childwidget
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS childwidget.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/widgets/childwidget
INSTALLS += target sources
QT += widgets


simulator: warning(This example might not fully work on Simulator platform)
