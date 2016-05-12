TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = semaphores \
                waitconditions

qtHaveModule(widgets): SUBDIRS += \
    mandelbrot \
    queuedcustomtype
