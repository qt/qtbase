TEMPLATE = subdirs
SUBDIRS = test # lackey should be moved to the QtScript module
!wince*:!symbian: SUBDIRS += example
symbian: TARGET.CAPABILITY = NetworkServices
