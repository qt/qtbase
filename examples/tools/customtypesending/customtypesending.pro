HEADERS   = message.h \
            window.h
SOURCES   = main.cpp \
            message.cpp \
            window.cpp
QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/customcompleter
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS customcompleter.pro resources
sources.path = $$[QT_INSTALL_EXAMPLES]/tools/customcompleter
INSTALLS += target sources


simulator: warning(This example might not fully work on Simulator platform)
