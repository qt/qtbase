HEADERS       = window.h \
                movementtransition.h
SOURCES       = main.cpp \
                window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine/rogue
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/statemachine/rogue
INSTALLS += target sources

QT += widgets

