TEMPLATE = subdirs
CONFIG += no_docs_target
requires(qtHaveModule(testlib))

SUBDIRS = auto

# benchmarks in debug mode is rarely sensible
# benchmarks are not sensible for code coverage (here with tool testcocoon)
!uikit:!testcocoon:contains(QT_CONFIG, release): SUBDIRS += benchmarks
