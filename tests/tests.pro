TEMPLATE = subdirs
SUBDIRS = auto

# benchmarks in debug mode is rarely sensible
contains(QT_CONFIG,release):SUBDIRS += benchmarks
