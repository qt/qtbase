TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    dbus \
    embedded \
    gestures \
    gui \
    ipc \
    network \
    opengl \
    qpa \
    qtconcurrent \
    qtestlib \
    sql \
    threads \
    tools \
    touch \
    widgets \
    xml

aggregate.files = aggregate/examples.pro
aggregate.path = $$[QT_INSTALL_EXAMPLES]
readme.files = README
readme.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += aggregate readme
