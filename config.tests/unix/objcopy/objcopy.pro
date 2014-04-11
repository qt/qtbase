SOURCES = objcopy.cpp
CONFIG -= qt

all.depends += only_keep_debug strip_debug add_gnu_debuglink

only_keep_debug.commands = $$QMAKE_OBJCOPY --only-keep-debug objcopy objcopy.debug
strip_debug.commands = $$QMAKE_OBJCOPY --strip-debug objcopy
add_gnu_debuglink.commands = $$QMAKE_OBJCOPY --add-gnu-debuglink=objcopy.debug objcopy

QMAKE_EXTRA_TARGETS += all only_keep_debug strip_debug add_gnu_debuglink
