TEMPLATE = subdirs
SUBDIRS += foo
foo.file = ./foo.pro
SUBDIRS += bar
bar.file = ./bar.pro
bar.depends += foo
