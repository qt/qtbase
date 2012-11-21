SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/widgets/windowlayout
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS windowlayout.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/tutorials/widgets/windowlayout
INSTALLS += target sources
QT += widgets


simulator: warning(This example might not fully work on Simulator platform)
