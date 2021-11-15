TEMPLATE = subdirs

SUBDIRS = \
    machtest_arm64.pro \
    machtest_x86_64.pro \
    machtest_fat.pro

machtest_fat-pro.depends = \
    machtest_arm64.pro \
    machtest_x86_64.pro

