HEADERS     = renderarea.h \
              window.h
SOURCES     = main.cpp \
              renderarea.cpp \
	      window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/transformations
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS transformations.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/transformations
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A64D
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
