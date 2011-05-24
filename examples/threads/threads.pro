TEMPLATE      = subdirs
SUBDIRS       = semaphores \
                waitconditions

!contains(QT_CONFIG, no-gui):SUBDIRS += mandelbrot

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/threads
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS threads.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/threads
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example
