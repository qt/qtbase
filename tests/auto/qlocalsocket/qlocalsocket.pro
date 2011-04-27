TEMPLATE = subdirs
SUBDIRS = lackey test
!wince*:!symbian: SUBDIRS += example
symbian: TARGET.CAPABILITY = NetworkServices
