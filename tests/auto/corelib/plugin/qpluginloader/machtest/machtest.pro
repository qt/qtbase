TEMPLATE = subdirs

SUBDIRS = \
    machtest_i386.pro \
    machtest_x86_64.pro \
    machtest_ppc64.pro \
    machtest_fat.pro

machtest_fat-pro.depends = \
    machtest_i386.pro \
    machtest_x86_64.pro \
    machtest_ppc64.pro

machtest_ppc64-pro.depends = \
    machtest_x86_64.pro
