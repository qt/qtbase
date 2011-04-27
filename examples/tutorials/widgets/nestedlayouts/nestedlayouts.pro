SOURCES = main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/widgets/nestedlayouts
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS nestedlayouts.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/widgets/nestedlayouts
INSTALLS += target sources
