# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs

# input doubleconversion
set(INPUT_doubleconversion "undefined" CACHE STRING "")
set_property(CACHE INPUT_doubleconversion PROPERTY STRINGS undefined no qt system)

# input libb2
set(INPUT_libb2 "undefined" CACHE STRING "")
set_property(CACHE INPUT_libb2 PROPERTY STRINGS undefined no qt system)



#### Libraries

if((UNIX AND NOT QNX) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    # QNX's libbacktrace has an API wholly different from all the other Unix
    # offerings
    qt_find_package(WrapBacktrace PROVIDED_TARGETS WrapBacktrace::WrapBacktrace MODULE_NAME core QMAKE_LIB backtrace)
endif()
qt_find_package(WrapSystemDoubleConversion
                PROVIDED_TARGETS WrapSystemDoubleConversion::WrapSystemDoubleConversion
                MODULE_NAME core QMAKE_LIB doubleconversion)
qt_find_package(GLIB2 PROVIDED_TARGETS GLIB2::GLIB2 MODULE_NAME core QMAKE_LIB glib)
qt_find_package(ICU 50.1 COMPONENTS i18n uc data PROVIDED_TARGETS ICU::i18n ICU::uc ICU::data
    MODULE_NAME core QMAKE_LIB icu)

if(QT_FEATURE_dlopen)
    qt_add_qmake_lib_dependency(icu libdl)
endif()
qt_find_package(Libsystemd PROVIDED_TARGETS PkgConfig::Libsystemd MODULE_NAME core QMAKE_LIB journald)
qt_find_package(WrapAtomic PROVIDED_TARGETS WrapAtomic::WrapAtomic MODULE_NAME core QMAKE_LIB libatomic)
qt_find_package(Libb2 PROVIDED_TARGETS Libb2::Libb2 MODULE_NAME core QMAKE_LIB libb2)
qt_find_package(WrapRt PROVIDED_TARGETS WrapRt::WrapRt MODULE_NAME core QMAKE_LIB librt)
qt_find_package(WrapSystemPCRE2 10.20 PROVIDED_TARGETS WrapSystemPCRE2::WrapSystemPCRE2 MODULE_NAME core QMAKE_LIB pcre2)
set_package_properties(WrapPCRE2 PROPERTIES TYPE REQUIRED)
if((QNX) OR QT_FIND_ALL_PACKAGES_ALWAYS)
    qt_find_package(PPS PROVIDED_TARGETS PPS::PPS MODULE_NAME core QMAKE_LIB pps)
endif()
qt_find_package(Slog2 PROVIDED_TARGETS Slog2::Slog2 MODULE_NAME core QMAKE_LIB slog2)


#### Tests

# atomicfptr
qt_config_compile_test(atomicfptr
    LABEL "working std::atomic for function pointers"
    CODE
"#include <atomic>
typedef void (*fptr)(int);
typedef std::atomic<fptr> atomicfptr;
void testfunction(int) { }
void test(volatile atomicfptr &a)
{
    fptr v = a.load(std::memory_order_acquire);
    while (!a.compare_exchange_strong(v, &testfunction,
                                      std::memory_order_acq_rel,
                                      std::memory_order_acquire)) {
        v = a.exchange(&testfunction);
    }
    a.store(&testfunction, std::memory_order_release);
}

int main(void)
{
    /* BEGIN TEST: */
atomicfptr fptr(testfunction);
test(fptr);
    /* END TEST: */
    return 0;
}
")

# clock-monotonic
qt_config_compile_test(clock_monotonic
    LABEL "POSIX monotonic clock"
    LIBRARIES
        WrapRt::WrapRt
    CODE
"#include <unistd.h>
#include <time.h>

int main(void)
{
    /* BEGIN TEST: */
#if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK-0 >= 0)
timespec ts;
clock_gettime(CLOCK_MONOTONIC, &ts);
#else
#  error Feature _POSIX_MONOTONIC_CLOCK not available
#endif
    /* END TEST: */
    return 0;
}
")

# close_range
qt_config_compile_test(close_range
    LABEL "close_range()"
    CODE
"#include <unistd.h>

int main()
{
    return close_range(3, 1024, 0) != 0;
}
")

# cloexec
qt_config_compile_test(cloexec
    LABEL "O_CLOEXEC"
    CODE
"#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    /* BEGIN TEST: */
int pipes[2];
(void) pipe2(pipes, O_CLOEXEC | O_NONBLOCK);
(void) fcntl(0, F_DUPFD_CLOEXEC, 0);
(void) dup3(0, 3, O_CLOEXEC);
#if defined(__NetBSD__)
(void) paccept(0, 0, 0, NULL, SOCK_CLOEXEC | SOCK_NONBLOCK);
#else
(void) accept4(0, 0, 0, SOCK_CLOEXEC | SOCK_NONBLOCK);
#endif
    /* END TEST: */
    return 0;
}
")

# cxx11_future
if (UNIX AND NOT ANDROID AND NOT QNX AND NOT INTEGRITY)
    set(cxx11_future_TEST_LIBRARIES pthread)
endif()
qt_config_compile_test(cxx11_future
    LABEL "C++11 <future>"
    LIBRARIES
     "${cxx11_future_TEST_LIBRARIES}"
    CODE
"#include <future>

int main(void)
{
    /* BEGIN TEST: */
std::future<int> f = std::async([]() { return 42; });
(void)f.get();
    /* END TEST: */
    return 0;
}
")

# cxx11_random
qt_config_compile_test(cxx11_random
    LABEL "C++11 <random>"
    CODE
"#include <random>

int main(void)
{
    /* BEGIN TEST: */
std::mt19937 mt(0);
    /* END TEST: */
    return 0;
}
")

# cxx17_filesystem
qt_config_compile_test(cxx17_filesystem
    LABEL "C++17 <filesystem>"
    CODE
"#include <filesystem>

int main(void)
{
    /* BEGIN TEST: */
std::filesystem::copy(
    std::filesystem::path(\"./file\"),
    std::filesystem::path(\"./other\"));
    /* END TEST: */
    return 0;
}
"
)

# dladdr
qt_config_compile_test(dladdr
    LABEL "dladdr"
    LIBRARIES
        dl
    CODE
"#define _GNU_SOURCE 1
#include <dlfcn.h>
int i = 0;
int main(void)
{
    Dl_info info;
    dladdr(&i, &info);
    return 0;
}"
)

# eventfd
qt_config_compile_test(eventfd
    LABEL "eventfd"
    CODE
"#include <sys/eventfd.h>

int main(void)
{
    /* BEGIN TEST: */
eventfd_t value;
int fd = eventfd(0, EFD_CLOEXEC);
eventfd_read(fd, &value);
eventfd_write(fd, value);
    /* END TEST: */
    return 0;
}
")

# futimens
qt_config_compile_test(futimens
    LABEL "futimens()"
    CODE
"#include <sys/stat.h>

int main(void)
{
    /* BEGIN TEST: */
futimens(-1, 0);
    /* END TEST: */
    return 0;
}
")

# getauxval
qt_config_compile_test(getauxval
    LABEL "getauxval()"
    CODE
"#include <sys/auxv.h>

int main(void)
{
    /* BEGIN TEST: */
(void) getauxval(AT_NULL);
    /* END TEST: */
    return 0;
}
")

# getentropy
qt_config_compile_test(getentropy
    LABEL "getentropy()"
    CODE
"#include <unistd.h>
#if __has_include(<sys/random.h>)
#  include <sys/random.h>
#endif

int main(void)
{
    /* BEGIN TEST: */
char buf[32];
(void) getentropy(buf, sizeof(buf));
    /* END TEST: */
    return 0;
}
")

# glibc
qt_config_compile_test(glibc
    LABEL "GNU libc"
    CODE
"#include <stdlib.h>

int main(void)
{
    /* BEGIN TEST: */
return __GLIBC__;
    /* END TEST: */
    return 0;
}
")

# inotify
qt_config_compile_test(inotify
    LABEL "inotify"
    CODE
"#include <sys/inotify.h>

int main(void)
{
    /* BEGIN TEST: */
inotify_init();
inotify_add_watch(0, \"foobar\", IN_ACCESS);
inotify_rm_watch(0, 1);
    /* END TEST: */
    return 0;
}
")

qt_config_compile_test(sysv_shm
    LABEL "System V/XSI shared memory"
    CODE
"#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>

int main(void)
{
    key_t unix_key = ftok(\"test\", 'Q');
    shmget(unix_key, 0, 0666 | IPC_CREAT | IPC_EXCL);
    shmctl(0, 0, (struct shmid_ds *)(0));
    return 0;
}
")

qt_config_compile_test(sysv_sem
    LABEL "System V/XSI semaphores"
    CODE
"#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>

int main(void)
{
    key_t unix_key = ftok(\"test\", 'Q');
    semctl(semget(unix_key, 1, 0666 | IPC_CREAT | IPC_EXCL), 0, IPC_RMID, 0);
    return 0;
}
")

if (LINUX)
    set(ipc_posix_TEST_LIBRARIES pthread WrapRt::WrapRt)
endif()
qt_config_compile_test(posix_shm
    LABEL "POSIX shared memory"
    LIBRARIES
     "${ipc_posix_TEST_LIBRARIES}"
    CODE
"#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(void)
{
    shm_open(\"test\", O_RDWR | O_CREAT | O_EXCL, 0666);
    shm_unlink(\"test\");
    return 0;
}
")

qt_config_compile_test(posix_sem
    LABEL "POSIX semaphores"
    LIBRARIES
     "${ipc_posix_TEST_LIBRARIES}"
    CODE
"#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>

int main(void)
{
    sem_close(sem_open(\"test\", O_CREAT | O_EXCL, 0666, 0));
    return 0;
}
")

# linkat
qt_config_compile_test(linkat
    LABEL "linkat()"
    CODE
"#define _ATFILE_SOURCE 1
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    /* BEGIN TEST: */
linkat(AT_FDCWD, \"foo\", AT_FDCWD, \"bar\", AT_SYMLINK_FOLLOW);
    /* END TEST: */
    return 0;
}
")

# ppoll
qt_config_compile_test(ppoll
    LABEL "ppoll()"
    CODE
"#include <signal.h>
#include <poll.h>

int main(void)
{
    /* BEGIN TEST: */
struct pollfd pfd;
struct timespec ts;
sigset_t sig;
ppoll(&pfd, 1, &ts, &sig);
    /* END TEST: */
    return 0;
}
")

# pollts
qt_config_compile_test(pollts
    LABEL "pollts()"
    CODE
"#include <poll.h>
#include <signal.h>
#include <time.h>

int main(void)
{
    /* BEGIN TEST: */
struct pollfd pfd;
struct timespec ts;
sigset_t sig;
pollts(&pfd, 1, &ts, &sig);
    /* END TEST: */
    return 0;
}
")

# poll
qt_config_compile_test(poll
    LABEL "poll()"
    CODE
"#include <poll.h>

int main(void)
{
    /* BEGIN TEST: */
struct pollfd pfd;
poll(&pfd, 1, 0);
    /* END TEST: */
    return 0;
}
")

# renameat2
qt_config_compile_test(renameat2
    LABEL "renameat2()"
    CODE
"#define _ATFILE_SOURCE 1
#include <fcntl.h>
#include <stdio.h>

int main(int, char **argv)
{
    /* BEGIN TEST: */
renameat2(AT_FDCWD, argv[1], AT_FDCWD, argv[2], RENAME_NOREPLACE | RENAME_WHITEOUT);
    /* END TEST: */
    return 0;
}
")

# statx
qt_config_compile_test(statx
    LABEL "statx() in libc"
    CODE
"#define _ATFILE_SOURCE 1
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    /* BEGIN TEST: */
struct statx statxbuf;
unsigned int mask = STATX_BASIC_STATS;
return statx(AT_FDCWD, \"\", AT_STATX_SYNC_AS_STAT, mask, &statxbuf);
    /* END TEST: */
    return 0;
}
")

# syslog
qt_config_compile_test(syslog
    LABEL "syslog"
    CODE
"#include <syslog.h>

int main(void)
{
    /* BEGIN TEST: */
openlog(\"qt\", 0, LOG_USER);
syslog(LOG_INFO, \"configure\");
closelog();
    /* END TEST: */
    return 0;
}
")

# cpp_winrt
qt_config_compile_test(cpp_winrt
    LABEL "cpp/winrt"
    LIBRARIES
        runtimeobject
    CODE
"// Including winrt/base.h causes an error in some configurations (Windows 10 SDK + c++20)
#   include <winrt/base.h>

int main(void)
{
    return 0;
}
")

# xlocalescanprint
qt_config_compile_test(xlocalescanprint
    LABEL "xlocale.h (or equivalents)"
    CODE
"#define QT_BEGIN_NAMESPACE
#define QT_END_NAMESPACE

#ifdef _MSVC_VER
#define Q_CC_MSVC _MSVC_VER
#endif

#define QT_NO_DOUBLECONVERSION

#include QDSP_P_H

int main(void)
{
    /* BEGIN TEST: */
#ifdef _MSVC_VER
_locale_t invalidLocale = NULL;
#else
locale_t invalidLocale = NULL;
#endif
double a = 3.4;
qDoubleSnprintf(argv[0], 1, invalidLocale, \"invalid format\", a);
qDoubleSscanf(argv[0], invalidLocale, \"invalid format\", &a, &argc);
    /* END TEST: */
    return 0;
}
"# FIXME: qmake: DEFINES += QDSP_P_H=$$shell_quote(\"@PWD@/text/qdoublescanprint_p.h\")
)



#### Features

qt_feature("clock-gettime" PRIVATE
    LABEL "clock_gettime()"
    CONDITION UNIX AND WrapRt_FOUND
)
qt_feature("clock-monotonic" PUBLIC
    LABEL "POSIX monotonic clock"
    CONDITION QT_FEATURE_clock_gettime AND TEST_clock_monotonic
)
qt_feature_definition("clock-monotonic" "QT_NO_CLOCK_MONOTONIC" NEGATE VALUE "1")
qt_feature("close_range" PRIVATE
    LABEL "close_range()"
    CONDITION QT_FEATURE_process AND TEST_close_range
    AUTODETECT UNIX
)
qt_feature("doubleconversion" PRIVATE
    LABEL "DoubleConversion"
)
qt_feature_definition("doubleconversion" "QT_NO_DOUBLECONVERSION" NEGATE VALUE "1")
qt_feature("system-doubleconversion" PRIVATE
    LABEL "  Using system DoubleConversion"
    CONDITION QT_FEATURE_doubleconversion AND WrapSystemDoubleConversion_FOUND
    ENABLE INPUT_doubleconversion STREQUAL 'system'
    DISABLE INPUT_doubleconversion STREQUAL 'qt'
)
qt_feature("cxx11_future" PUBLIC
    LABEL "C++11 <future>"
    CONDITION TEST_cxx11_future
)
qt_feature("cxx17_filesystem" PUBLIC
    LABEL "C++17 <filesystem>"
    CONDITION TEST_cxx17_filesystem
)
qt_feature("dladdr" PRIVATE
    LABEL "dladdr"
    CONDITION QT_FEATURE_dlopen AND TEST_dladdr
)
qt_feature("eventfd" PUBLIC
    LABEL "eventfd"
    CONDITION NOT WASM AND TEST_eventfd
)
qt_feature_definition("eventfd" "QT_NO_EVENTFD" NEGATE VALUE "1")
qt_feature("futimens" PRIVATE
    LABEL "futimens()"
    CONDITION NOT WIN32 AND TEST_futimens
)
qt_feature("getauxval" PRIVATE
    LABEL "getauxval()"
    CONDITION LINUX AND TEST_getauxval
)
qt_feature("getentropy" PRIVATE
    LABEL "getentropy()"
    CONDITION UNIX AND TEST_getentropy
)
qt_feature("glib" PUBLIC PRIVATE
    LABEL "GLib"
    AUTODETECT NOT WIN32
    CONDITION GLIB2_FOUND
)
qt_feature_definition("glib" "QT_NO_GLIB" NEGATE VALUE "1")
qt_feature("glibc" PRIVATE
    LABEL "GNU libc"
    AUTODETECT ( LINUX OR HURD )
    CONDITION TEST_glibc
)
qt_feature("icu" PRIVATE
    LABEL "ICU"
    AUTODETECT NOT WIN32
    CONDITION ICU_FOUND
)
qt_feature("inotify" PUBLIC PRIVATE
    LABEL "inotify"
    CONDITION TEST_inotify
)
qt_feature_definition("inotify" "QT_NO_INOTIFY" NEGATE VALUE "1")
qt_feature("ipc_posix"
    LABEL "Defaulting legacy IPC to POSIX"
    CONDITION TEST_posix_shm AND TEST_posix_sem AND (
        FEATURE_ipc_posix OR (APPLE AND QT_FEATURE_appstore_compliant)
        OR NOT TEST_sysv_shm OR NOT TEST_sysv_sem
    )
)
qt_feature_definition("ipc_posix" "QT_POSIX_IPC")
qt_feature("journald" PRIVATE
    LABEL "journald"
    AUTODETECT OFF
    CONDITION Libsystemd_FOUND
)
# Used by QCryptographicHash for the BLAKE2 hashing algorithms
qt_feature("system-libb2" PRIVATE
    LABEL "Using system libb2"
    CONDITION Libb2_FOUND
    ENABLE INPUT_libb2 STREQUAL 'system'
    DISABLE INPUT_libb2 STREQUAL 'no' OR INPUT_libb2 STREQUAL 'qt'
)
# Currently only used by QTemporaryFile; linkat() exists on Android, but hardlink creation fails due to security rules
qt_feature("linkat" PRIVATE
    LABEL "linkat()"
    AUTODETECT ( LINUX AND NOT ANDROID ) OR HURD
    CONDITION TEST_linkat
)
qt_feature("std-atomic64" PUBLIC
    LABEL "64 bit atomic operations"
    CONDITION WrapAtomic_FOUND
)
qt_feature("mimetype" PUBLIC
    SECTION "Utilities"
    LABEL "Mimetype handling"
    PURPOSE "Provides MIME type handling."
)
qt_feature_definition("mimetype" "QT_NO_MIMETYPE" NEGATE VALUE "1")
qt_feature("mimetype-database" PRIVATE
    LABEL "Built-in copy of the MIME database"
    CONDITION QT_FEATURE_mimetype
)
qt_feature("pcre2"
    LABEL "PCRE2"
    ENABLE INPUT_pcre STREQUAL 'qt' OR QT_FEATURE_system_pcre2
    DISABLE INPUT_pcre STREQUAL 'no'
)
qt_feature_config("pcre2" QMAKE_PRIVATE_CONFIG)
qt_feature("system-pcre2" PRIVATE
    LABEL "  Using system PCRE2"
    CONDITION WrapSystemPCRE2_FOUND
    ENABLE INPUT_pcre STREQUAL 'system'
    DISABLE INPUT_pcre STREQUAL 'no' OR INPUT_pcre STREQUAL 'qt'
)
qt_feature("poll_ppoll" PRIVATE
    LABEL "Native ppoll()"
    CONDITION NOT WASM AND TEST_ppoll
    EMIT_IF NOT WIN32
)
qt_feature("poll_pollts" PRIVATE
    LABEL "Native pollts()"
    CONDITION NOT QT_FEATURE_poll_ppoll AND TEST_pollts
    EMIT_IF NOT WIN32
)
qt_feature("poll_poll" PRIVATE
    LABEL "Native poll()"
    CONDITION NOT QT_FEATURE_poll_ppoll AND NOT QT_FEATURE_poll_pollts AND TEST_poll
    EMIT_IF NOT WIN32
)
qt_feature("poll_select" PRIVATE
    LABEL "Emulated poll()"
    CONDITION NOT QT_FEATURE_poll_ppoll AND NOT QT_FEATURE_poll_pollts AND NOT QT_FEATURE_poll_poll
    EMIT_IF NOT WIN32
)
qt_feature_definition("poll_select" "QT_NO_NATIVE_POLL")
qt_feature("posix_sem" PRIVATE
    LABEL "POSIX semaphores"
    CONDITION TEST_posix_sem
)
qt_feature("posix_shm" PRIVATE
    LABEL "POSIX shared memory"
    CONDITION TEST_posix_shm AND UNIX
)
qt_feature("qqnx_pps" PRIVATE
    LABEL "PPS"
    CONDITION PPS_FOUND
    EMIT_IF QNX
)
qt_feature("renameat2" PRIVATE
    LABEL "renameat2()"
    CONDITION ( LINUX OR HURD ) AND TEST_renameat2
)
qt_feature("slog2" PRIVATE
    LABEL "slog2"
    CONDITION Slog2_FOUND
)
qt_feature("statx" PRIVATE
    LABEL "statx() in libc"
    CONDITION ( LINUX OR HURD ) AND TEST_statx
)
qt_feature("syslog" PRIVATE
    LABEL "syslog"
    AUTODETECT OFF
    CONDITION TEST_syslog
)
qt_feature("sysv_sem" PRIVATE
    LABEL "System V / XSI semaphores"
    CONDITION TEST_sysv_sem
)
qt_feature("sysv_shm" PRIVATE
    LABEL "System V / XSI shared memory"
    CONDITION TEST_sysv_shm
)
qt_feature("threadsafe-cloexec"
    LABEL "Threadsafe pipe creation"
    CONDITION TEST_cloexec
)
qt_feature_definition("threadsafe-cloexec" "QT_THREADSAFE_CLOEXEC" VALUE "1")
qt_feature_config("threadsafe-cloexec" QMAKE_PUBLIC_QT_CONFIG)
qt_feature("regularexpression" PUBLIC
    SECTION "Kernel"
    LABEL "QRegularExpression"
    PURPOSE "Provides an API to Perl-compatible regular expressions."
    CONDITION QT_FEATURE_system_pcre2 OR QT_FEATURE_pcre2
)
qt_feature_definition("regularexpression" "QT_NO_REGULAREXPRESSION" NEGATE VALUE "1")
qt_feature("backtrace" PRIVATE
    LABEL "backtrace"
    CONDITION UNIX AND QT_FEATURE_regularexpression AND WrapBacktrace_FOUND
)
qt_feature("sharedmemory" PUBLIC
    SECTION "Kernel"
    LABEL "QSharedMemory"
    PURPOSE "Provides access to a shared memory segment."
    CONDITION WIN32 OR TEST_sysv_shm OR TEST_posix_shm
)
qt_feature_definition("sharedmemory" "QT_NO_SHAREDMEMORY" NEGATE VALUE "1")
qt_feature("shortcut" PUBLIC
    SECTION "Kernel"
    LABEL "QShortcut"
    PURPOSE "Provides keyboard accelerators and shortcuts."
)
qt_feature_definition("shortcut" "QT_NO_SHORTCUT" NEGATE VALUE "1")
qt_feature("systemsemaphore" PUBLIC
    SECTION "Kernel"
    LABEL "QSystemSemaphore"
    PURPOSE "Provides a general counting system semaphore."
    CONDITION WIN32 OR TEST_sysv_sem OR TEST_posix_sem
)
qt_feature_definition("systemsemaphore" "QT_NO_SYSTEMSEMAPHORE" NEGATE VALUE "1")
qt_feature("xmlstream" PUBLIC
    SECTION "Kernel"
    LABEL "XML Streaming APIs"
    PURPOSE "Provides a simple streaming API for XML."
)
qt_feature("cpp-winrt" PRIVATE PUBLIC
    LABEL "cpp/winrt base"
    PURPOSE "basic cpp/winrt language projection support"
    AUTODETECT WIN32
    CONDITION WIN32 AND TEST_cpp_winrt
)
qt_feature("xmlstreamreader" PUBLIC
    SECTION "Kernel"
    LABEL "QXmlStreamReader"
    PURPOSE "Provides a well-formed XML parser with a simple streaming API."
    CONDITION QT_FEATURE_xmlstream
)
qt_feature("xmlstreamwriter" PUBLIC
    SECTION "Kernel"
    LABEL "QXmlStreamWriter"
    PURPOSE "Provides a XML writer with a simple streaming API."
    CONDITION QT_FEATURE_xmlstream
)
qt_feature("textdate" PUBLIC
    SECTION "Data structures"
    LABEL "Text Date"
    PURPOSE "Supports month and day names in dates."
)
qt_feature_definition("textdate" "QT_NO_TEXTDATE" NEGATE VALUE "1")
qt_feature("datestring" PUBLIC
    SECTION "Data structures"
    LABEL "QDate/QTime/QDateTime"
    PURPOSE "Provides conversion between dates and strings."
    CONDITION QT_FEATURE_textdate
)
qt_feature_definition("datestring" "QT_NO_DATESTRING" NEGATE VALUE "1")
qt_feature("process" PUBLIC
    SECTION "File I/O"
    LABEL "QProcess"
    PURPOSE "Supports external process invocation."
    CONDITION QT_FEATURE_processenvironment
              AND (QT_FEATURE_thread OR NOT UNIX)
              AND NOT UIKIT
              AND NOT INTEGRITY
              AND NOT VXWORKS
              AND NOT rtems
              AND NOT WASM
)
qt_feature_definition("process" "QT_NO_PROCESS" NEGATE VALUE "1")
qt_feature("processenvironment" PUBLIC
    SECTION "File I/O"
    LABEL "QProcessEnvironment"
    PURPOSE "Provides a higher-level abstraction of environment variables."
    CONDITION NOT INTEGRITY AND NOT rtems
)
qt_feature("temporaryfile" PUBLIC
    SECTION "File I/O"
    LABEL "QTemporaryFile"
    PURPOSE "Provides an I/O device that operates on temporary files."
)
qt_feature_definition("temporaryfile" "QT_NO_TEMPORARYFILE" NEGATE VALUE "1")
qt_feature("library" PUBLIC
    SECTION "File I/O"
    LABEL "QLibrary"
    PURPOSE "Provides a wrapper for dynamically loaded libraries."
    CONDITION WIN32 OR HPUX OR QT_FEATURE_dlopen
)
qt_feature_definition("library" "QT_NO_LIBRARY" NEGATE VALUE "1")
qt_feature("settings" PUBLIC
    SECTION "File I/O"
    LABEL "QSettings"
    PURPOSE "Provides persistent application settings."
)
qt_feature_definition("settings" "QT_NO_SETTINGS" NEGATE VALUE "1")
qt_feature("filesystemwatcher" PUBLIC
    SECTION "File I/O"
    LABEL "QFileSystemWatcher"
    PURPOSE "Provides an interface for monitoring files and directories for modifications."
)
qt_feature_definition("filesystemwatcher" "QT_NO_FILESYSTEMWATCHER" NEGATE VALUE "1")
qt_feature("filesystemiterator" PUBLIC
    SECTION "File I/O"
    LABEL "QFileSystemIterator"
    PURPOSE "Provides fast file system iteration."
)
qt_feature_definition("filesystemiterator" "QT_NO_FILESYSTEMITERATOR" NEGATE VALUE "1")
qt_feature("itemmodel" PUBLIC
    SECTION "ItemViews"
    LABEL "Qt Item Model"
    PURPOSE "Provides the item model for item views"
)
qt_feature_definition("itemmodel" "QT_NO_ITEMMODEL" NEGATE VALUE "1")
qt_feature("proxymodel" PUBLIC
    SECTION "ItemViews"
    LABEL "QAbstractProxyModel"
    PURPOSE "Supports processing of data passed between another model and a view."
    CONDITION QT_FEATURE_itemmodel
)
qt_feature_definition("proxymodel" "QT_NO_PROXYMODEL" NEGATE VALUE "1")
qt_feature("sortfilterproxymodel" PUBLIC
    SECTION "ItemViews"
    LABEL "QSortFilterProxyModel"
    PURPOSE "Supports sorting and filtering of data passed between another model and a view."
    CONDITION QT_FEATURE_proxymodel AND QT_FEATURE_regularexpression
)
qt_feature_definition("sortfilterproxymodel" "QT_NO_SORTFILTERPROXYMODEL" NEGATE VALUE "1")
qt_feature("identityproxymodel" PUBLIC
    SECTION "ItemViews"
    LABEL "QIdentityProxyModel"
    PURPOSE "Supports proxying a source model unmodified."
    CONDITION QT_FEATURE_proxymodel
)
qt_feature_definition("identityproxymodel" "QT_NO_IDENTITYPROXYMODEL" NEGATE VALUE "1")
qt_feature("transposeproxymodel" PUBLIC
    SECTION "ItemViews"
    LABEL "QTransposeProxyModel"
    PURPOSE "Provides a proxy to swap rows and columns of a model."
    CONDITION QT_FEATURE_proxymodel
)
qt_feature_definition("transposeproxymodel" "QT_NO_TRANSPOSEPROXYMODEL" NEGATE VALUE "1")
qt_feature("concatenatetablesproxymodel" PUBLIC
    SECTION "ItemViews"
    LABEL "QConcatenateTablesProxyModel"
    PURPOSE "Supports concatenating source models."
    CONDITION QT_FEATURE_proxymodel
)
qt_feature_definition("concatenatetablesproxymodel" "QT_NO_CONCATENATETABLESPROXYMODEL" NEGATE VALUE "1")
qt_feature("stringlistmodel" PUBLIC
    SECTION "ItemViews"
    LABEL "QStringListModel"
    PURPOSE "Provides a model that supplies strings to views."
    CONDITION QT_FEATURE_itemmodel
)
qt_feature_definition("stringlistmodel" "QT_NO_STRINGLISTMODEL" NEGATE VALUE "1")
qt_feature("translation" PUBLIC
    SECTION "Internationalization"
    LABEL "Translation"
    PURPOSE "Supports translations using QObject::tr()."
)
qt_feature_definition("translation" "QT_NO_TRANSLATION" NEGATE VALUE "1")
qt_feature("easingcurve" PUBLIC
    SECTION "Utilities"
    LABEL "Easing curve"
    PURPOSE "Provides easing curve."
)
qt_feature("animation" PUBLIC
    SECTION "Utilities"
    LABEL "Animation"
    PURPOSE "Provides a framework for animations."
    CONDITION QT_FEATURE_easingcurve
)
qt_feature_definition("animation" "QT_NO_ANIMATION" NEGATE VALUE "1")
qt_feature("gestures" PUBLIC
    SECTION "Utilities"
    LABEL "Gesture"
    PURPOSE "Provides a framework for gestures."
)
qt_feature_definition("gestures" "QT_NO_GESTURES" NEGATE VALUE "1")
qt_feature("sha3-fast" PRIVATE
    SECTION "Utilities"
    LABEL "Speed optimized SHA3"
    PURPOSE "Optimizes SHA3 for speed instead of size."
)
qt_feature("jalalicalendar" PUBLIC
    SECTION "Utilities"
    LABEL "QJalaliCalendar"
    PURPOSE "Support the Jalali (Persian) calendar"
)
qt_feature("hijricalendar" PRIVATE
    SECTION "Utilities"
    LABEL "QHijriCalendar"
    PURPOSE "Generic basis for Islamic calendars, providing shared locale data"
)
qt_feature("islamiccivilcalendar" PUBLIC
    SECTION "Utilities"
    LABEL "QIslamicCivilCalendar"
    PURPOSE "Support the Islamic Civil calendar"
    CONDITION QT_FEATURE_hijricalendar
)
qt_feature("timezone" PUBLIC
    SECTION "Utilities"
    LABEL "QTimeZone"
    PURPOSE "Provides support for time-zone handling."
    CONDITION NOT WASM
)
qt_feature("datetimeparser" PRIVATE
    SECTION "Utilities"
    LABEL "QDateTimeParser"
    PURPOSE "Provides support for parsing date-time texts."
    CONDITION QT_FEATURE_datestring
)
qt_feature("commandlineparser" PUBLIC
    SECTION "Utilities"
    LABEL "QCommandlineParser"
    PURPOSE "Provides support for command line parsing."
)
qt_feature("lttng" PRIVATE
    LABEL "LTTNG"
    AUTODETECT OFF
    CONDITION LINUX AND LTTNGUST_FOUND
    ENABLE INPUT_trace STREQUAL 'lttng' OR ( INPUT_trace STREQUAL 'yes' AND LINUX )
    DISABLE INPUT_trace STREQUAL 'etw' OR INPUT_trace STREQUAL 'no'
)
qt_feature("etw" PRIVATE
    LABEL "ETW"
    AUTODETECT OFF
    CONDITION WIN32
    ENABLE INPUT_trace STREQUAL 'etw' OR ( INPUT_trace STREQUAL 'yes' AND WIN32 )
    DISABLE INPUT_trace STREQUAL 'lttng' OR INPUT_trace STREQUAL 'no'
)
qt_feature("ctf" PRIVATE
    LABEL "CTF"
    AUTODETECT OFF
    ENABLE INPUT_trace STREQUAL 'ctf'
    DISABLE INPUT_trace STREQUAL 'etw' OR INPUT_trace STREQUAL 'no' OR INPUT_trace STREQUAL 'lttng'
)
qt_feature("forkfd_pidfd" PRIVATE
    LABEL "CLONE_PIDFD support in forkfd"
    CONDITION LINUX
)
qt_feature("cborstreamreader" PUBLIC
    SECTION "Utilities"
    LABEL "CBOR stream reading"
    PURPOSE "Provides support for reading the CBOR binary format.  Note that this is required for plugin loading. Qt GUI needs QPA plugins for basic operation."
)
qt_feature("cborstreamwriter" PUBLIC
    SECTION "Utilities"
    LABEL "CBOR stream writing"
    PURPOSE "Provides support for writing the CBOR binary format."
)
qt_feature("poll-exit-on-error" PRIVATE
    LABEL "Poll exit on error"
    AUTODETECT OFF
    CONDITION UNIX
    PURPOSE "Exit on error instead of just printing the error code and continue."
)
qt_feature("permissions" PUBLIC
    SECTION "Utilities"
    LABEL "Application permissions"
    PURPOSE "Provides support for requesting user permission to access restricted data or APIs"
)
qt_feature("openssl-hash" PRIVATE
    LABEL "OpenSSL based cryptographic hash"
    AUTODETECT OFF
    CONDITION QT_FEATURE_openssl_linked AND QT_FEATURE_opensslv30
    PURPOSE "Uses OpenSSL based implementation of cryptographic hash algorithms."
)

qt_configure_add_summary_section(NAME "Qt Core")
qt_configure_add_summary_entry(ARGS "backtrace")
qt_configure_add_summary_entry(ARGS "doubleconversion")
qt_configure_add_summary_entry(ARGS "system-doubleconversion")
qt_configure_add_summary_entry(ARGS "forkfd_pidfd" CONDITION LINUX)
qt_configure_add_summary_entry(ARGS "glib")
qt_configure_add_summary_entry(ARGS "icu")
qt_configure_add_summary_entry(ARGS "system-libb2")
qt_configure_add_summary_entry(ARGS "mimetype-database")
qt_configure_add_summary_entry(ARGS "permissions")
qt_configure_add_summary_entry(ARGS "ipc_posix" CONDITION UNIX)
qt_configure_add_summary_entry(
    TYPE "firstAvailableFeature"
    ARGS "etw lttng ctf"
    MESSAGE "Tracing backend"
)
qt_configure_add_summary_entry(ARGS "openssl-hash")
qt_configure_add_summary_section(NAME "Logging backends")
qt_configure_add_summary_entry(ARGS "journald")
qt_configure_add_summary_entry(ARGS "syslog")
qt_configure_add_summary_entry(ARGS "slog2")
qt_configure_end_summary_section() # end of "Logging backends" section
qt_configure_add_summary_entry(
    ARGS "qqnx_pps"
    CONDITION QNX
)
qt_configure_add_summary_entry(ARGS "pcre2")
qt_configure_add_summary_entry(ARGS "system-pcre2")
qt_configure_end_summary_section() # end of "Qt Core" section
qt_configure_add_report_entry(
    TYPE NOTE
    MESSAGE "journald, syslog or slog2 integration is enabled.  If your users intend to develop applications against this build, ensure that the IDEs they use either set QT_FORCE_STDERR_LOGGING to 1 or are able to read the logged output from journald, syslog or slog2."
    CONDITION QT_FEATURE_journald OR QT_FEATURE_syslog OR ( QNX AND QT_FEATURE_slog2 )
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "C++11 <random> is required and is missing or failed to compile."
    CONDITION NOT TEST_cxx11_random
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "Your C library does not provide sscanf_l or snprintf_l.  You need to use libdouble-conversion for double/string conversion."
    CONDITION INPUT_doubleconversion STREQUAL 'no' AND NOT TEST_xlocalescanprint
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "detected a std::atomic implementation that fails for function pointers.  Please apply the patch corresponding to your Standard Library vendor, found in qtbase/config.tests/atomicfptr"
    CONDITION NOT TEST_atomicfptr
)
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "Qt requires poll(), ppoll(), poll_ts() or select() on this platform"
    CONDITION ( UNIX OR INTEGRITY ) AND ( NOT QT_FEATURE_poll_ppoll ) AND ( NOT QT_FEATURE_poll_pollts ) AND ( NOT QT_FEATURE_poll_poll ) AND ( NOT QT_FEATURE_poll_select )
)
qt_configure_add_report_entry(
    TYPE WARNING
    MESSAGE "Basic cpp/winrt support missing. Some features might not be available."
    CONDITION WIN32 AND NOT QT_FEATURE_cpp_winrt
)
