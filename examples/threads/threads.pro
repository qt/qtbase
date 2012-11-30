TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = semaphores \
                waitconditions

!contains(QT_CONFIG, no-widgets):SUBDIRS += mandelbrot
