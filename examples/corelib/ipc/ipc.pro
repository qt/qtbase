requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
# no QSharedMemory
!vxworks:SUBDIRS = sharedmemory
qtHaveModule(network): SUBDIRS += localfortuneserver localfortuneclient
