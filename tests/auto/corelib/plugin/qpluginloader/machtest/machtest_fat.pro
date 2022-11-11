TEMPLATE = aux
OTHER_FILES += generate-bad.pl

# Needs explicit load()ing due to aux template. Relies on QT being non-empty.
load(qt)

# Generate a fat binary with three architectures
fat_all.target = good.fat.all.dylib
fat_all.commands = lipo -create -output $@ \
                              -arch arm64 good.arm64.dylib \
                              -arch x86_64 good.x86_64.dylib
fat_all.depends += good.arm64.dylib good.x86_64.dylib

fat_no_arm64.target = good.fat.no-arm64.dylib
fat_no_arm64.commands = lipo -create -output $@ -arch x86_64 good.x86_64.dylib
fat_no_arm64.depends += good.x86_64.dylib

fat_no_x86_64.target = good.fat.no-x86_64.dylib
fat_no_x86_64.commands = lipo -create -output $@ -arch arm64 good.arm64.dylib
fat_no_x86_64.depends += good.arm64.dylib

stub_flags = -L$$system(xcrun --show-sdk-path)/usr/lib/ -stdlib=libc++

stub_arm64.commands = $$QMAKE_CXX $$stub_flags -shared -arch arm64 -o stub.arm64.dylib $$PWD/stub.cpp
stub_arm64.depends += $$PWD/stub.cpp

stub_x86_64.commands = $$QMAKE_CXX $$stub_flags -shared -arch x86_64 -o stub.x86_64.dylib $$PWD/stub.cpp
stub_x86_64.depends += $$PWD/stub.cpp

fat_stub_arm64.target = good.fat.stub-arm64.dylib
fat_stub_arm64.commands = lipo -create -output $@ -arch arm64 stub.arm64.dylib
fat_stub_arm64.depends += stub_arm64 good.x86_64.dylib

fat_stub_x86_64.target = good.fat.stub-x86_64.dylib
fat_stub_x86_64.commands = lipo -create -output $@ -arch x86_64 stub.x86_64.dylib
fat_stub_x86_64.depends += stub_x86_64 good.arm64.dylib

bad.commands = $$PWD/generate-bad.pl
bad.depends += $$PWD/generate-bad.pl

MYTARGETS = $$fat_all.depends fat_all fat_no_x86_64 fat_no_arm64 \
            fat_stub_arm64 fat_stub_x86_64 bad stub_arm64 stub_x86_64
all.depends += $$MYTARGETS
QMAKE_EXTRA_TARGETS += $$MYTARGETS all

QMAKE_CLEAN += $$fat_all.target $$fat_no_arm64.target $$fat_no_x86_64.target \
            $$fat_stub_arm64.target $$fat_stub_x86_64.target \
            "bad*.dylib"
