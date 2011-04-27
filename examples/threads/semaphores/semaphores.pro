SOURCES += semaphores.cpp
QT = core
CONFIG -= app_bundle
CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/threads/semaphores
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS semaphores.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/threads/semaphores
INSTALLS += target sources

symbian: CONFIG += qt_example
