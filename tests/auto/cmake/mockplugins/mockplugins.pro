TEMPLATE = subdirs

src_mock1plugin.subdir = $$PWD/mock1plugin
src_mock1plugin.target = sub-mockplugin1
src_mock1plugin.depends = mockplugins1

src_mock2plugin.subdir = $$PWD/mock2plugin
src_mock2plugin.target = sub-mockplugin2
src_mock2plugin.depends = mockplugins1

src_mock3plugin.subdir = $$PWD/mock3plugin
src_mock3plugin.target = sub-mockplugin3
src_mock3plugin.depends = mockplugins1

src_mock4plugin.subdir = $$PWD/mock4plugin
src_mock4plugin.target = sub-mockplugin4
src_mock4plugin.depends = mockplugins1

src_mock5plugin.subdir = $$PWD/mock5plugin
src_mock5plugin.target = sub-mockplugin5
src_mock5plugin.depends = mockplugins3

src_mock6plugin.subdir = $$PWD/mock6plugin
src_mock6plugin.target = sub-mockplugin6
src_mock6plugin.depends = mockplugins3

SUBDIRS += \
    mockplugins1 \
    mockplugins2 \
    mockplugins3 \
    src_mock1plugin \
    src_mock2plugin \
    src_mock3plugin \
    src_mock4plugin \
    src_mock5plugin \
    src_mock6plugin
