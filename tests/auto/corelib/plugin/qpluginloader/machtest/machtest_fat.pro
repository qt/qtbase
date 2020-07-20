TEMPLATE = aux
OTHER_FILES += generate-bad.pl

# Needs explicit load()ing due to aux template. Relies on QT being non-empty.
load(qt)

# Generate a fat binary with three architectures
fat_all.target = good.fat.all.dylib
fat_all.commands = lipo -create -output $@ \
                              -arch ppc64 good.ppc64.dylib \
                              -arch i386 good.i386.dylib \
                              -arch x86_64 good.x86_64.dylib
fat_all.depends += good.i386.dylib good.x86_64.dylib good.ppc64.dylib

fat_no_i386.target = good.fat.no-i386.dylib
fat_no_i386.commands = lipo -create -output $@ -arch x86_64 good.x86_64.dylib -arch ppc64 good.ppc64.dylib
fat_no_i386.depends += good.x86_64.dylib good.ppc64.dylib

fat_no_x86_64.target = good.fat.no-x86_64.dylib
fat_no_x86_64.commands = lipo -create -output $@ -arch i386 good.i386.dylib -arch ppc64 good.ppc64.dylib
fat_no_x86_64.depends += good.i386.dylib good.ppc64.dylib

stub_i386.commands = $$QMAKE_CXX -shared -arch i386 -o stub.i386.dylib $$PWD/stub.cpp
stub_i386.depends += $$PWD/stub.cpp

stub_x86_64.commands = $$QMAKE_CXX -shared -arch x86_64 -o stub.x86_64.dylib $$PWD/stub.cpp
stub_x86_64.depends += $$PWD/stub.cpp

fat_stub_i386.target = good.fat.stub-i386.dylib
fat_stub_i386.commands = lipo -create -output $@ -arch ppc64 good.ppc64.dylib -arch i386 stub.i386.dylib
fat_stub_i386.depends += stub_i386 good.x86_64.dylib good.ppc64.dylib

fat_stub_x86_64.target = good.fat.stub-x86_64.dylib
fat_stub_x86_64.commands = lipo -create -output $@ -arch ppc64 good.ppc64.dylib -arch x86_64 stub.x86_64.dylib
fat_stub_x86_64.depends += stub_x86_64 good.i386.dylib good.ppc64.dylib

bad.commands = $$PWD/generate-bad.pl
bad.depends += $$PWD/generate-bad.pl

MYTARGETS = $$fat_all.depends fat_all fat_no_x86_64 fat_no_i386 \
            fat_stub_i386 fat_stub_x86_64 bad stub_i386 stub_x86_64
all.depends += $$MYTARGETS
QMAKE_EXTRA_TARGETS += $$MYTARGETS all

QMAKE_CLEAN += $$fat_all.target $$fat_no_i386.target $$fat_no_x86_64.target \
            $$fat_stub_i386.target $$fat_stub_x86_64.target \
            "bad*.dylib"
