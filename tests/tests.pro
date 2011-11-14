TEMPLATE = subdirs
SUBDIRS = auto

# benchmarks in debug mode is rarely sensible
# benchmarks are not sensible for code coverage (here with tool testcocoon)
!testcocoon:contains(QT_CONFIG,release):SUBDIRS += benchmarks

# disable 'make check' on Mac OS X for the time being
mac: auto.CONFIG += no_check_target
