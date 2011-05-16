TEMPLATE = subdirs
SUBDIRS  = subdir.pro

extratarget.CONFIG         = recursive
extratarget.recurse        = $$SUBDIRS
extratarget.recurse_target = extratarget
QMAKE_EXTRA_TARGETS       += extratarget
