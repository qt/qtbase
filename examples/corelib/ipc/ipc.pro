requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
# no QSharedMemory
!vxworks:!integrity: SUBDIRS = sharedmemory
qtHaveModule(network): SUBDIRS += localfortuneserver localfortuneclient
