HEADERS       = window.h
SOURCES       = main.cpp \
                window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/spinboxes
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS spinboxes.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/spinboxes
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
