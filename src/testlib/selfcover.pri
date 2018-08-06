# Configuration for testlib and its tests, to instrument with
# FrogLogic's Squish CoCo (cf. testcocoon.prf, which handles similar
# for general code; but testlib needs special handling).

# Only for use when feature testlib_selfcover is enabled:
!qtConfig(testlib_selfcover): return()

# This enables verification that testlib itself is adequately tested,
# as a grounds for trusting that testing with it is useful.
# Exclude all non-testlib source from coverage instrumentation:
COVERAGE_OPTIONS = --cs-exclude-file-abs-wildcard=$$QT_SOURCE_TREE/*
COVERAGE_OPTIONS += --cs-include-file-abs-wildcard=*/src/testlib/*
COVERAGE_OPTIONS += --cs-mcc # enable Multiple Condition Coverage
COVERAGE_OPTIONS += --cs-mcdc # enable Multiple Condition / Decision Coverage
# (recommended for ISO 26262 ASIL A, B and C -- highly recommended for ASIL D)
# https://doc.froglogic.com/squish-coco/4.1/codecoverage.html#sec%3Amcdc

QMAKE_CFLAGS += $$COVERAGE_OPTIONS
QMAKE_CXXFLAGS += $$COVERAGE_OPTIONS
QMAKE_LFLAGS += $$COVERAGE_OPTIONS

# FIXME: relies on QMAKE_* being just the command-names, with no path prefix
QMAKE_CC = cs$$QMAKE_CC
QMAKE_CXX = cs$$QMAKE_CXX
QMAKE_LINK = cs$$QMAKE_LINK
QMAKE_LINK_SHLIB = cs$$QMAKE_LINK_SHLIB
QMAKE_AR = cs$$QMAKE_AR
QMAKE_LIB = cs$$QMAKE_LIB
