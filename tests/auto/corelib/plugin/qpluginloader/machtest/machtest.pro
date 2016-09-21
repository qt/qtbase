TEMPLATE = aux
OTHER_FILES += \
    ppcconverter.pl \
    generate-bad.pl

# Needs explicit load()ing due to aux template. Relies on QT being non-empty.
load(qt)

i386_d.target = good.i386.dylib
i386_d.depends = EXPORT_VALID_ARCHS=i386
i386.target = good.i386.dylib
i386.commands = $(CXX) $(CXXFLAGS) -shared -o $@ -I$(INCPATH) $<
i386.depends += $$PWD/../fakeplugin.cpp

x86_64_d.target = good.x86_64.dylib
x86_64_d.depends = EXPORT_VALID_ARCHS=x86_64
x86_64.target = good.x86_64.dylib
x86_64.commands = $(CXX) $(CXXFLAGS) -shared -o $@ -I$(INCPATH) $<
x86_64.depends += $$PWD/../fakeplugin.cpp

# Current Mac OS X toolchains have no compiler for PPC anymore
# So we fake it by converting an x86-64 binary to (little-endian!) PPC64
ppc64.target = good.ppc64.dylib
ppc64.commands = $$PWD/ppcconverter.pl $< $@
ppc64.depends = x86_64 $$PWD/ppcconverter.pl

# Generate a fat binary with three architectures
fat_all.target = good.fat.all.dylib
fat_all.commands = lipo -create -output $@ \
                              -arch ppc64 $$ppc64.target \
                              -arch i386 $$i386.target \
                              -arch x86_64 $$x86_64.target
fat_all.depends += i386 x86_64 ppc64

fat_no_i386.target = good.fat.no-i386.dylib
fat_no_i386.commands = lipo -create -output $@ -arch x86_64 $$x86_64.target -arch ppc64 $$ppc64.target
fat_no_i386.depends += x86_64 ppc64

fat_no_x86_64.target = good.fat.no-x86_64.dylib
fat_no_x86_64.commands = lipo -create -output $@ -arch i386 $$i386.target -arch ppc64 $$ppc64.target
fat_no_x86_64.depends += i386 ppc64

fat_stub_i386.target = good.fat.stub-i386.dylib
fat_stub_i386.commands = lipo -create -output $@ -arch ppc64 $$ppc64.target -arch_blank i386
fat_stub_i386.depends += x86_64 ppc64

fat_stub_x86_64.target = good.fat.stub-x86_64.dylib
fat_stub_x86_64.commands = lipo -create -output $@ -arch ppc64 $$ppc64.target -arch_blank x86_64
fat_stub_x86_64.depends += i386 ppc64

bad.commands = $$PWD/generate-bad.pl
bad.depends += $$PWD/generate-bad.pl

MYTARGETS = $$fat_all.depends fat_all fat_no_x86_64 fat_no_i386 \
            fat_stub_i386 fat_stub_x86_64 bad
all.depends += $$MYTARGETS
QMAKE_EXTRA_TARGETS += i386_d x86_64_d $$MYTARGETS all

QMAKE_CLEAN += $$i386.target $$x86_64.target $$ppc64.target $$fat_all.target \
            $$fat_no_i386.target $$fat_no_x86_64.target \
            $$fat_stub_i386.target $$fat_stub_x86_64.target \
            "bad*.dylib"


