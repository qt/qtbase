requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
# no QSharedMemory
!vxworks:SUBDIRS = sharedmemory
!wince:qtHaveModule(network): SUBDIRS += localfortuneserver localfortuneclient
