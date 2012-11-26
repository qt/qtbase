SOURCES += main.cpp lighting.cpp
HEADERS += lighting.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/lighting
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS lighting.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/lighting
INSTALLS += target sources
QT += widgets


