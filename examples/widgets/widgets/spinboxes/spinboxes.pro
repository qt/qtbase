HEADERS       = window.h
SOURCES       = main.cpp \
                window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/spinboxes
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS spinboxes.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/spinboxes
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
