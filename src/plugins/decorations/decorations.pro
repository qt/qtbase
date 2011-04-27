TEMPLATE = subdirs
contains(decoration-plugins, default)	:SUBDIRS += default
contains(decoration-plugins, styled)	:SUBDIRS += styled
contains(decoration-plugins, windows)	:SUBDIRS += windows
