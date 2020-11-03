# Additional Qt project file for QtEntryPoint lib
!win32:error("$$_FILE_ is intended only for Windows!")

TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += entrypoint_module.pro
SUBDIRS += entrypoint_implementation.pro
