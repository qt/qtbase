QMAKE_APPLE_DEVICE_ARCHS = ppc64
include(machtest.pri)

OTHER_FILES += ppcconverter.pl

# Current macOS toolchains have no compiler for PPC anymore
# So we fake it by converting an x86-64 binary to (little-endian!) PPC64
goodlib.commands = $$PWD/ppcconverter.pl $< $@
goodlib.depends = good.x86_64.dylib $$PWD/ppcconverter.pl
