isEmpty(QT_ARCH) {
    # We're most likely being parsed in a fromfile() call, in which case the
    # QMake environment isn't complete. Load qt_config in an attempt to set
    # the variables we need (QT_ARCH and CONFIG, in particular).
    load(qt_config)
}

equals(QT_ARCH, x86_64)|contains(CONFIG, x86_64):CONFIG += arch_x86_64
else:equals(QT_ARCH, "i386"):CONFIG += arch_i386
else:equals(QT_ARCH, "arm"):CONFIG += arch_arm
else:equals(QMAKE_HOST.arch, armv7l):CONFIG += arch_arm
else:equals(QMAKE_HOST.arch, armv5tel):CONFIG += arch_arm
else:equals(QMAKE_HOST.arch, x86_64):CONFIG += arch_x86_64
else:equals(QMAKE_HOST.arch, x86):CONFIG += arch_i386
else:equals(QMAKE_HOST.arch, i386):CONFIG += arch_i386
else:equals(QMAKE_HOST.arch, i686):CONFIG += arch_i386
else:error("Couldn't detect supported architecture ($$QMAKE_HOST.arch/$$QT_ARCH). Currently supported architectures are: x64, x86 and arm")

include($$PWD/v8base.pri)

# In debug-and-release builds, generated sources must not go to the same
# directory, or they could clobber each other in highly parallelized builds
CONFIG(debug, debug|release):V8_GENERATED_SOURCES_DIR = generated-debug
else:                        V8_GENERATED_SOURCES_DIR = generated-release

!contains(QT_CONFIG, static): DEFINES += V8_SHARED BUILDING_V8_SHARED

# this maybe removed in future
DEFINES += ENABLE_DEBUGGER_SUPPORT

# this is needed by crankshaft ( http://code.google.com/p/v8/issues/detail?id=1271 )
DEFINES += ENABLE_VMSTATE_TRACKING ENABLE_LOGGING_AND_PROFILING

CONFIG(debug, debug|release) {
    DEFINES += DEBUG V8_ENABLE_CHECKS OBJECT_PRINT ENABLE_DISASSEMBLER
} else {
    DEFINES += NDEBUG
}

V8SRC = $$V8DIR/src

INCLUDEPATH += \
    $$V8SRC

SOURCES += \
    $$V8SRC/accessors.cc \
    $$V8SRC/allocation.cc \
    $$V8SRC/api.cc \
    $$V8SRC/assembler.cc \
    $$V8SRC/ast.cc \
    $$V8SRC/atomicops_internals_x86_gcc.cc \
    $$V8SRC/bignum.cc \
    $$V8SRC/bignum-dtoa.cc \
    $$V8SRC/bootstrapper.cc \
    $$V8SRC/builtins.cc \
    $$V8SRC/cached-powers.cc \
    $$V8SRC/checks.cc \
    $$V8SRC/circular-queue.cc \
    $$V8SRC/code-stubs.cc \
    $$V8SRC/codegen.cc \
    $$V8SRC/compilation-cache.cc \
    $$V8SRC/compiler.cc \
    $$V8SRC/contexts.cc \
    $$V8SRC/conversions.cc \
    $$V8SRC/counters.cc \
    $$V8SRC/cpu-profiler.cc \
    $$V8SRC/data-flow.cc \
    $$V8SRC/dateparser.cc \
    $$V8SRC/debug-agent.cc \
    $$V8SRC/debug.cc \
    $$V8SRC/deoptimizer.cc \
    $$V8SRC/disassembler.cc \
    $$V8SRC/diy-fp.cc \
    $$V8SRC/dtoa.cc \
    $$V8SRC/elements.cc \
    $$V8SRC/execution.cc \
    $$V8SRC/factory.cc \
    $$V8SRC/flags.cc \
    $$V8SRC/frames.cc \
    $$V8SRC/full-codegen.cc \
    $$V8SRC/func-name-inferrer.cc \
    $$V8SRC/gdb-jit.cc \
    $$V8SRC/global-handles.cc \
    $$V8SRC/fast-dtoa.cc \
    $$V8SRC/fixed-dtoa.cc \
    $$V8SRC/handles.cc \
    $$V8SRC/hashmap.cc \
    $$V8SRC/heap-profiler.cc \
    $$V8SRC/heap.cc \
    $$V8SRC/hydrogen.cc \
    $$V8SRC/hydrogen-instructions.cc \
    $$V8SRC/ic.cc \
    $$V8SRC/incremental-marking.cc \
    $$V8SRC/inspector.cc \
    $$V8SRC/interpreter-irregexp.cc \
    $$V8SRC/isolate.cc \
    $$V8SRC/jsregexp.cc \
    $$V8SRC/lithium-allocator.cc \
    $$V8SRC/lithium.cc \
    $$V8SRC/liveedit.cc \
    $$V8SRC/liveobjectlist.cc \
    $$V8SRC/log-utils.cc \
    $$V8SRC/log.cc \
    $$V8SRC/mark-compact.cc \
    $$V8SRC/messages.cc \
    $$V8SRC/objects.cc \
    $$V8SRC/objects-printer.cc \
    $$V8SRC/objects-visiting.cc \
    $$V8SRC/parser.cc \
    $$V8SRC/preparser.cc \
    $$V8SRC/preparse-data.cc \
    $$V8SRC/profile-generator.cc \
    $$V8SRC/property.cc \
    $$V8SRC/regexp-macro-assembler-irregexp.cc \
    $$V8SRC/regexp-macro-assembler.cc \
    $$V8SRC/regexp-stack.cc \
    $$V8SRC/rewriter.cc \
    $$V8SRC/runtime.cc \
    $$V8SRC/runtime-profiler.cc \
    $$V8SRC/safepoint-table.cc \
    $$V8SRC/scanner.cc \
    $$V8SRC/scanner-character-streams.cc \
    $$V8SRC/scopeinfo.cc \
    $$V8SRC/scopes.cc \
    $$V8SRC/serialize.cc \
    $$V8SRC/snapshot-common.cc \
    $$V8SRC/spaces.cc \
    $$V8SRC/string-search.cc \
    $$V8SRC/string-stream.cc \
    $$V8SRC/strtod.cc \
    $$V8SRC/stub-cache.cc \
    $$V8SRC/token.cc \
    $$V8SRC/type-info.cc \
    $$V8SRC/unicode.cc \
    $$V8SRC/utils.cc \
    $$V8SRC/v8-counters.cc \
    $$V8SRC/v8.cc \
    $$V8SRC/v8conversions.cc \
    $$V8SRC/v8threads.cc \
    $$V8SRC/v8utils.cc \
    $$V8SRC/variables.cc \
    $$V8SRC/version.cc \
    $$V8SRC/store-buffer.cc \
    $$V8SRC/zone.cc \
    $$V8SRC/extensions/gc-extension.cc \
    $$V8SRC/extensions/externalize-string-extension.cc

SOURCES += \
    $$V8SRC/snapshot-empty.cc \

arch_arm {
DEFINES += V8_TARGET_ARCH_ARM
SOURCES += \
    $$V8SRC/arm/builtins-arm.cc \
    $$V8SRC/arm/code-stubs-arm.cc \
    $$V8SRC/arm/codegen-arm.cc \
    $$V8SRC/arm/constants-arm.cc \
    $$V8SRC/arm/cpu-arm.cc \
    $$V8SRC/arm/debug-arm.cc \
    $$V8SRC/arm/deoptimizer-arm.cc \
    $$V8SRC/arm/disasm-arm.cc \
    $$V8SRC/arm/frames-arm.cc \
    $$V8SRC/arm/full-codegen-arm.cc \
    $$V8SRC/arm/ic-arm.cc \
    $$V8SRC/arm/lithium-arm.cc \
    $$V8SRC/arm/lithium-codegen-arm.cc \
    $$V8SRC/arm/lithium-gap-resolver-arm.cc \
    $$V8SRC/arm/macro-assembler-arm.cc \
    $$V8SRC/arm/regexp-macro-assembler-arm.cc \
    $$V8SRC/arm/stub-cache-arm.cc \
    $$V8SRC/arm/assembler-arm.cc
}

arch_i386 {
DEFINES += V8_TARGET_ARCH_IA32
SOURCES += \
    $$V8SRC/ia32/assembler-ia32.cc \
    $$V8SRC/ia32/builtins-ia32.cc \
    $$V8SRC/ia32/code-stubs-ia32.cc \
    $$V8SRC/ia32/codegen-ia32.cc \
    $$V8SRC/ia32/cpu-ia32.cc \
    $$V8SRC/ia32/debug-ia32.cc \
    $$V8SRC/ia32/deoptimizer-ia32.cc \
    $$V8SRC/ia32/disasm-ia32.cc \
    $$V8SRC/ia32/frames-ia32.cc \
    $$V8SRC/ia32/full-codegen-ia32.cc \
    $$V8SRC/ia32/ic-ia32.cc \
    $$V8SRC/ia32/lithium-codegen-ia32.cc \
    $$V8SRC/ia32/lithium-gap-resolver-ia32.cc \
    $$V8SRC/ia32/lithium-ia32.cc \
    $$V8SRC/ia32/macro-assembler-ia32.cc \
    $$V8SRC/ia32/regexp-macro-assembler-ia32.cc \
    $$V8SRC/ia32/stub-cache-ia32.cc
}

# FIXME Should we use QT_CONFIG instead? What about 32 bit Macs?
arch_x86_64 {
DEFINES += V8_TARGET_ARCH_X64
SOURCES += \
    $$V8SRC/x64/assembler-x64.cc \
    $$V8SRC/x64/builtins-x64.cc \
    $$V8SRC/x64/code-stubs-x64.cc \
    $$V8SRC/x64/codegen-x64.cc \
    $$V8SRC/x64/cpu-x64.cc \
    $$V8SRC/x64/debug-x64.cc \
    $$V8SRC/x64/deoptimizer-x64.cc \
    $$V8SRC/x64/disasm-x64.cc \
    $$V8SRC/x64/frames-x64.cc \
    $$V8SRC/x64/full-codegen-x64.cc \
    $$V8SRC/x64/ic-x64.cc \
    $$V8SRC/x64/lithium-codegen-x64.cc \
    $$V8SRC/x64/lithium-gap-resolver-x64.cc \
    $$V8SRC/x64/lithium-x64.cc \
    $$V8SRC/x64/macro-assembler-x64.cc \
    $$V8SRC/x64/regexp-macro-assembler-x64.cc \
    $$V8SRC/x64/stub-cache-x64.cc
}

unix:!symbian:!macx {
SOURCES += \
    $$V8SRC/platform-linux.cc \
    $$V8SRC/platform-posix.cc
}

#os:macos
macx {
SOURCES += \
    $$V8SRC/platform-macos.cc \
    $$V8SRC/platform-posix.cc
}

win32 {
SOURCES += \
    $$V8SRC/platform-win32.cc
LIBS += Ws2_32.lib Winmm.lib
win32-msvc*: QMAKE_CXXFLAGS += -wd4100 -wd 4291 -wd4351 -wd4355 -wd4800
win32-msvc*:arch_i386: DEFINES += _USE_32BIT_TIME_T
}

#mode:debug
CONFIG(debug) {
    SOURCES += \
        $$V8SRC/objects-debug.cc \
        $$V8SRC/prettyprinter.cc \
        $$V8SRC/regexp-macro-assembler-tracer.cc
}

V8_LIBRARY_FILES = \
    $$V8SRC/runtime.js \
    $$V8SRC/v8natives.js \
    $$V8SRC/array.js \
    $$V8SRC/string.js \
    $$V8SRC/uri.js \
    $$V8SRC/math.js \
    $$V8SRC/messages.js \
    $$V8SRC/apinatives.js \
    $$V8SRC/date.js \
    $$V8SRC/regexp.js \
    $$V8SRC/json.js \
    $$V8SRC/liveedit-debugger.js \
    $$V8SRC/mirror-debugger.js \
    $$V8SRC/debug-debugger.js

V8_EXPERIMENTAL_LIBRARY_FILES = \
    $$V8SRC/proxy.js \

v8_js2c.commands = python $$V8DIR/tools/js2c.py $$V8_GENERATED_SOURCES_DIR/libraries.cpp CORE off
v8_js2c.commands += $$V8SRC/macros.py ${QMAKE_FILE_IN}
v8_js2c.output = $$V8_GENERATED_SOURCES_DIR/libraries.cpp
v8_js2c.input = V8_LIBRARY_FILES
v8_js2c.variable_out = SOURCES
v8_js2c.dependency_type = TYPE_C
v8_js2c.depends = $$V8DIR/tools/js2c.py $$V8SRC/macros.py
v8_js2c.CONFIG += combine
v8_js2c.name = generating[v8] ${QMAKE_FILE_IN}
silent:v8_js2c.commands = @echo generating[v8] ${QMAKE_FILE_IN} && $$v8_js2c.commands

v8_js2c_experimental.commands = python $$V8DIR/tools/js2c.py $$V8_GENERATED_SOURCES_DIR/experimental-libraries.cpp EXPERIMENTAL off
v8_js2c_experimental.commands += $$V8SRC/macros.py ${QMAKE_FILE_IN}
v8_js2c_experimental.output = $$V8_GENERATED_SOURCES_DIR/experimental-libraries.cpp
v8_js2c_experimental.input = V8_EXPERIMENTAL_LIBRARY_FILES
v8_js2c_experimental.variable_out = SOURCES
v8_js2c_experimental.dependency_type = TYPE_C
v8_js2c_experimental.depends = $$V8DIR/tools/js2c.py $$V8SRC/macros.py
v8_js2c_experimental.CONFIG += combine
v8_js2c_experimental.name = generating[v8] ${QMAKE_FILE_IN}

QMAKE_EXTRA_COMPILERS += v8_js2c v8_js2c_experimental
