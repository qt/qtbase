requires(qtHaveModule(widgets))

TEMPLATE      = subdirs

qtConfig(sharedmemory): SUBDIRS = sharedmemory
qtHaveModule(network): SUBDIRS += localfortuneserver localfortuneclient
