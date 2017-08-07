TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS = \
    corelib \
    embedded \
    qpa \
    touch

qtHaveModule(dbus): SUBDIRS += dbus
qtHaveModule(network): SUBDIRS += network
qtHaveModule(testlib): SUBDIRS += qtestlib
qtHaveModule(concurrent): SUBDIRS += qtconcurrent
qtHaveModule(sql): SUBDIRS += sql
qtHaveModule(widgets): SUBDIRS += widgets
qtHaveModule(xml): SUBDIRS += xml
qtHaveModule(gui): SUBDIRS += gui
qtHaveModule(gui):qtConfig(opengl): SUBDIRS += opengl

aggregate.files = aggregate/examples.pro
aggregate.path = $$[QT_INSTALL_EXAMPLES]
readme.files = README
readme.path = $$[QT_INSTALL_EXAMPLES]
INSTALLS += aggregate readme
