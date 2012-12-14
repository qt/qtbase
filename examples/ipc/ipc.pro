TEMPLATE      = subdirs
# no QSharedMemory
!vxworks:!qnx:SUBDIRS = sharedmemory
!wince*: SUBDIRS += localfortuneserver localfortuneclient
