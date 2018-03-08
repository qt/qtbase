TEMPLATE = subdirs

SUBDIRS = test
!vxworks{
SUBDIRS += stressTest
test.depends = stressTest
}
requires(qtConfig(private_tests))
