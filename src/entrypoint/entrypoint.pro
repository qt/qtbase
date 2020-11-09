
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += entrypoint_module.pro

win32 {
    SUBDIRS += entrypoint_implementation.pro
}
