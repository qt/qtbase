requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
# no QSharedMemory
!vxworks:!integrity: SUBDIRS = sharedmemory
!wince:qtHaveModule(network): SUBDIRS += localfortuneserver localfortuneclient
