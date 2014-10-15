TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    corelib \
    dbus \
    embedded \
    gui \
    network \
    qpa \
    qtconcurrent \
    qtestlib \
    sql \
    touch \
    widgets \
    xml

contains(QT_CONFIG, opengl): SUBDIRS += opengl

aggregate.files = aggregate/examples.pro
aggregate.path = $$[QT_INSTALL_EXAMPLES]
readme.files = README
readme.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += aggregate readme
