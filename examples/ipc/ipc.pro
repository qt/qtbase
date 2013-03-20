requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
# no QSharedMemory
!vxworks:!qnx:SUBDIRS = sharedmemory
!wince*:qtHaveModule(network): SUBDIRS += localfortuneserver localfortuneclient
