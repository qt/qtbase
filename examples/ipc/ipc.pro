TEMPLATE      = subdirs
# no QSharedMemory
!vxworks:!qnx:SUBDIRS = sharedmemory
!wince*: SUBDIRS += localfortuneserver localfortuneclient

# install
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS ipc.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc
INSTALLS += sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
