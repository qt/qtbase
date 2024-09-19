// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/qtestcase.h>
#include <QtTest/private/qtestcrashhandler_p.h>
#include <QtTest/qtestassert.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qfloat16.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qlist.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qtemporarydir.h>
#include <QtCore/qthread.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/private/qlocking_p.h>
#include <QtCore/private/qtools_p.h>
#include <QtCore/private/qwaitcondition_p.h>

#include <QtCore/qtestsupport_core.h>

#include <QtTest/private/qtestlog_p.h>
#include <QtTest/private/qtesttable_p.h>
#include <QtTest/qtestdata.h>
#include <QtTest/private/qtestresult_p.h>
#include <QtTest/private/qsignaldumper_p.h>
#include <QtTest/private/qbenchmark_p.h>
#if QT_CONFIG(batch_test_support)
#include <QtTest/private/qtestregistry_p.h>
#endif  // QT_CONFIG(batch_test_support)
#include <QtTest/private/cycle_p.h>
#include <QtTest/private/qtestblacklist_p.h>
#if defined(HAVE_XCTEST)
#include <QtTest/private/qxctestlogger_p.h>
#endif
#if defined Q_OS_MACOS
#include <QtTest/private/qtestutil_macos_p.h>
#endif

#if defined(Q_OS_DARWIN)
#include <QtTest/private/qappletestlogger_p.h>
#endif

#if !defined(Q_OS_INTEGRITY) || __GHS_VERSION_NUMBER > 202014
#  include <charconv>
#else
// Broken implementation, causes link failures just by #include'ing!
#  undef __cpp_lib_to_chars     // in case <version> was included
#endif

#include <stdio.h>
#include <stdlib.h>

#if defined(Q_OS_LINUX)
#include <sys/prctl.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#ifdef Q_OS_UNIX
#include <QtCore/private/qcore_unix_p.h>

#include <errno.h>
#if __has_include(<paths.h>)
# include <paths.h>
#endif
#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
# if !defined(Q_OS_INTEGRITY)
#  include <sys/resource.h>
# endif
# ifndef _PATH_DEFPATH
#  define _PATH_DEFPATH     "/usr/bin:/bin"
# endif
# ifndef SIGSTKSZ
#  define SIGSTKSZ          0       /* we have code to set the minimum */
# endif
# ifndef SA_RESETHAND
#  define SA_RESETHAND      0
# endif
#endif

#if defined(Q_OS_MACOS)
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <CoreFoundation/CFPreferences.h>
#endif

#if defined(Q_OS_WASM)
#include <emscripten.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QTest {
namespace CrashHandler {
#if defined(Q_OS_UNIX) && (!defined(Q_OS_WASM) || QT_CONFIG(thread))
struct iovec IoVec(struct iovec vec)
{
    return vec;
}
struct iovec IoVec(const char *str)
{
    struct iovec r = {};
    r.iov_base = const_cast<char *>(str);
    r.iov_len = strlen(str);
    return r;
}

struct iovec asyncSafeToString(int n, AsyncSafeIntBuffer &&result)
{
    char *ptr = result.array.data();
    if (false) {
#ifdef __cpp_lib_to_chars
    } else if (auto r = std::to_chars(ptr, ptr + result.array.size(), n, 10); r.ec == std::errc{}) {
        ptr = r.ptr;
#endif
    } else {
        // handle the sign
        if (n < 0) {
            *ptr++ = '-';
            n = -n;
        }

        // find the highest power of the base that is less than this number
        static constexpr int StartingDivider = ([]() {
            int divider = 1;
            for (int i = 0; i < std::numeric_limits<int>::digits10; ++i)
                divider *= 10;
            return divider;
        }());
        int divider = StartingDivider;
        while (divider && n < divider)
            divider /= 10;

        // now convert to string
        while (divider > 1) {
            int quot = n / divider;
            n = n % divider;
            divider /= 10;
            *ptr++ = quot + '0';
        }
        *ptr++ = n + '0';
    }

#ifndef QT_NO_DEBUG
    // this isn't necessary, it just helps in the debugger
    *ptr = '\0';
#endif
    struct iovec r;
    r.iov_base = result.array.data();
    r.iov_len = ptr - result.array.data();
    return r;
};
#endif // defined(Q_OS_UNIX) && (!defined(Q_OS_WASM) || QT_CONFIG(thread))

bool alreadyDebugging()
{
#if defined(Q_OS_LINUX)
    int fd = open("/proc/self/status", O_RDONLY);
    if (fd == -1)
        return false;
    char buffer[2048];
    ssize_t size = read(fd, buffer, sizeof(buffer) - 1);
    if (size == -1) {
        close(fd);
        return false;
    }
    buffer[size] = 0;
    const char tracerPidToken[] = "\nTracerPid:";
    char *tracerPid = strstr(buffer, tracerPidToken);
    if (!tracerPid) {
        close(fd);
        return false;
    }
    tracerPid += sizeof(tracerPidToken);
    long int pid = strtol(tracerPid, &tracerPid, 10);
    close(fd);
    return pid != 0;
#elif defined(Q_OS_WIN)
    return IsDebuggerPresent();
#elif defined(Q_OS_MACOS)
    // Check if there is an exception handler for the process:
    mach_msg_type_number_t portCount = 0;
    exception_mask_t masks[EXC_TYPES_COUNT];
    mach_port_t ports[EXC_TYPES_COUNT];
    exception_behavior_t behaviors[EXC_TYPES_COUNT];
    thread_state_flavor_t flavors[EXC_TYPES_COUNT];
    exception_mask_t mask = EXC_MASK_ALL & ~(EXC_MASK_RESOURCE | EXC_MASK_GUARD);
    kern_return_t result = task_get_exception_ports(mach_task_self(), mask, masks, &portCount,
                                                    ports, behaviors, flavors);
    if (result == KERN_SUCCESS) {
        for (mach_msg_type_number_t portIndex = 0; portIndex < portCount; ++portIndex) {
            if (MACH_PORT_VALID(ports[portIndex])) {
                return true;
            }
        }
    }
    return false;
#else
    // TODO
    return false;
#endif
}

namespace {
enum DebuggerProgram { None, Gdb, Lldb };
static bool hasSystemCrashReporter()
{
#if defined(Q_OS_MACOS)
    return QTestPrivate::macCrashReporterWillShowDialog();
#else
    return false;
#endif
}
} // unnamed namespaced

void maybeDisableCoreDump()
{
#ifdef RLIMIT_CORE
    bool ok = false;
    const int disableCoreDump = qEnvironmentVariableIntValue("QTEST_DISABLE_CORE_DUMP", &ok);
    if (ok && disableCoreDump) {
        struct rlimit limit;
        limit.rlim_cur = 0;
        limit.rlim_max = 0;
        if (setrlimit(RLIMIT_CORE, &limit) != 0)
            qWarning("Failed to disable core dumps: %d", errno);
    }
#endif
}

static DebuggerProgram debugger = None;
void prepareStackTrace()
{

    bool ok = false;
    const int disableStackDump = qEnvironmentVariableIntValue("QTEST_DISABLE_STACK_DUMP", &ok);
    if (ok && disableStackDump)
        return;

    if (hasSystemCrashReporter())
        return;

#if defined(Q_OS_MACOS)
    #define CSR_ALLOW_UNRESTRICTED_FS (1 << 1)
    std::optional<uint32_t> sipConfiguration = qt_mac_sipConfiguration();
    if (!sipConfiguration || !(*sipConfiguration & CSR_ALLOW_UNRESTRICTED_FS))
        return; // LLDB will fail to provide a valid stack trace
#endif

#ifdef Q_OS_UNIX
    // like QStandardPaths::findExecutable(), but simpler
    auto hasExecutable = [](const char *execname) {
        std::string candidate;
        std::string path;
        if (const char *p = getenv("PATH"); p && *p)
            path = p;
        else
            path = _PATH_DEFPATH;
        for (const char *p = std::strtok(&path[0], ":'"); p; p = std::strtok(nullptr, ":")) {
            candidate = p;
            candidate += '/';
            candidate += execname;
            if (QT_ACCESS(candidate.data(), X_OK) == 0)
                return true;
        }
        return false;
    };

    static constexpr DebuggerProgram debuggerSearchOrder[] = {
#  if defined(Q_OS_QNX) || (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
        Gdb, Lldb
#  else
        Lldb, Gdb
#  endif
    };
    for (DebuggerProgram candidate : debuggerSearchOrder) {
        switch (candidate) {
        case None:
            Q_UNREACHABLE();
            break;
        case Gdb:
            if (hasExecutable("gdb")) {
                debugger = Gdb;
                return;
            }
            break;
        case Lldb:
            if (hasExecutable("lldb")) {
                debugger = Lldb;
                return;
            }
            break;
        }
    }
#endif // Q_OS_UNIX
}

#if !defined(Q_OS_WASM) || QT_CONFIG(thread)
void printTestRunTime()
{
    const int msecsFunctionTime = qRound(QTestLog::msecsFunctionTime());
    const int msecsTotalTime = qRound(QTestLog::msecsTotalTime());
    const char *const name = QTest::currentTestFunction();
    writeToStderr("\n         ", name ? name : "[Non-test]",
                  " function time: ", asyncSafeToString(msecsFunctionTime),
                  "ms, total time: ", asyncSafeToString(msecsTotalTime), "ms\n");
}

void generateStackTrace()
{
    if (debugger == None || alreadyDebugging())
        return;

#  if defined(Q_OS_LINUX) && defined(PR_SET_PTRACER)
    // allow ourselves to be debugged
    (void) prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY);
#  endif

#  if defined(Q_OS_UNIX) && !defined(Q_OS_WASM) && !defined(Q_OS_INTEGRITY) && !defined(Q_OS_VXWORKS)
    writeToStderr("\n=== Stack trace ===\n");

    // execlp() requires null-termination, so call the default constructor
    AsyncSafeIntBuffer pidbuffer;
    asyncSafeToString(getpid(), std::move(pidbuffer));

    // Note: POSIX.1-2001 still has fork() in the list of async-safe functions,
    // but in a future edition, it might be removed. It would be safer to wake
    // up a babysitter thread to launch the debugger.
    pid_t pid = fork();
    if (pid == 0) {
        // child process
        (void) dup2(STDERR_FILENO, STDOUT_FILENO); // redirect stdout to stderr

        switch (debugger) {
        case None:
            Q_UNREACHABLE();
            break;
        case Gdb:
            execlp("gdb", "gdb", "--nx", "--batch", "-ex", "thread apply all bt",
                   "--pid", pidbuffer.array.data(), nullptr);
            break;
        case Lldb:
            execlp("lldb", "lldb", "--no-lldbinit", "--batch", "-o", "bt all",
                   "--attach-pid", pidbuffer.array.data(), nullptr);
            break;
        }
        _exit(1);
    } else if (pid < 0) {
        writeToStderr("Failed to start debugger.\n");
    } else {
        int ret;
        QT_EINTR_LOOP(ret, waitpid(pid, nullptr, 0));
    }

    writeToStderr("=== End of stack trace ===\n");
#  endif // Q_OS_UNIX && !Q_OS_WASM && !Q_OS_INTEGRITY && !Q_OS_VXWORKS
}
#endif  // !defined(Q_OS_WASM) || QT_CONFIG(thread)

#if defined(Q_OS_WIN)
void blockUnixSignals()
{
  // Windows does have C signals, but doesn't use them for the purposes we're
  // talking about here
}
#elif defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
void blockUnixSignals()
{
    // Block most Unix signals so the WatchDog thread won't be called when
    // external signals are delivered, thus avoiding interfering with the test
    sigset_t set;
    sigfillset(&set);

    // we allow the crashing signals, in case we have bugs
    for (int signo : FatalSignalHandler::fatalSignals)
        sigdelset(&set, signo);

    pthread_sigmask(SIG_BLOCK, &set, nullptr);
}
#endif // Q_OS_* choice

#if defined(Q_OS_WIN)
void DebugSymbolResolver::cleanup()
{
    if (m_dbgHelpLib)
        FreeLibrary(m_dbgHelpLib);
    m_dbgHelpLib = 0;
    m_symFromAddr = nullptr;
}

DebugSymbolResolver::DebugSymbolResolver(HANDLE process)
    : m_process(process), m_dbgHelpLib(0), m_symFromAddr(nullptr)
{
    bool success = false;
    m_dbgHelpLib = LoadLibraryW(L"dbghelp.dll");
    if (m_dbgHelpLib) {
        SymInitializeType symInitialize = reinterpret_cast<SymInitializeType>(
            reinterpret_cast<QFunctionPointer>(GetProcAddress(m_dbgHelpLib, "SymInitialize")));
        m_symFromAddr = reinterpret_cast<SymFromAddrType>(
            reinterpret_cast<QFunctionPointer>(GetProcAddress(m_dbgHelpLib, "SymFromAddr")));
        success = symInitialize && m_symFromAddr && symInitialize(process, NULL, TRUE);
    }
    if (!success)
        cleanup();
}

DebugSymbolResolver::Symbol DebugSymbolResolver::resolveSymbol(DWORD64 address) const
{
    // reserve additional buffer where SymFromAddr() will store the name
    struct NamedSymbolInfo : public DBGHELP_SYMBOL_INFO {
        enum { symbolNameLength = 255 };

        char name[symbolNameLength + 1];
    };

    Symbol result;
    if (!isValid())
        return result;
    NamedSymbolInfo symbolBuffer;
    memset(&symbolBuffer, 0, sizeof(NamedSymbolInfo));
    symbolBuffer.MaxNameLen = NamedSymbolInfo::symbolNameLength;
    symbolBuffer.SizeOfStruct = sizeof(DBGHELP_SYMBOL_INFO);
    if (!m_symFromAddr(m_process, address, 0, &symbolBuffer))
        return result;
    result.name = qstrdup(symbolBuffer.Name);
    result.address = symbolBuffer.Address;
    return result;
}

WindowsFaultHandler::WindowsFaultHandler()
{
#  if !defined(Q_CC_MINGW)
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#  endif
    SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(windowsFaultHandler);
}

LONG WINAPI WindowsFaultHandler::windowsFaultHandler(struct _EXCEPTION_POINTERS *exInfo)
{
    enum { maxStackFrames = 100 };
    char appName[MAX_PATH];
    if (!GetModuleFileNameA(NULL, appName, MAX_PATH))
        appName[0] = 0;
    const int msecsFunctionTime = qRound(QTestLog::msecsFunctionTime());
    const int msecsTotalTime = qRound(QTestLog::msecsTotalTime());
    const void *exceptionAddress = exInfo->ExceptionRecord->ExceptionAddress;
    fprintf(stderr, "A crash occurred in %s.\n", appName);
    if (const char *name = QTest::currentTestFunction())
        fprintf(stderr, "While testing %s\n", name);
    fprintf(stderr, "Function time: %dms Total time: %dms\n\n"
                    "Exception address: 0x%p\n"
                    "Exception code   : 0x%lx\n",
            msecsFunctionTime, msecsTotalTime, exceptionAddress,
            exInfo->ExceptionRecord->ExceptionCode);

    DebugSymbolResolver resolver(GetCurrentProcess());
    if (resolver.isValid()) {
        DebugSymbolResolver::Symbol exceptionSymbol = resolver.resolveSymbol(DWORD64(exceptionAddress));
        if (exceptionSymbol.name) {
            fprintf(stderr, "Nearby symbol    : %s\n", exceptionSymbol.name);
            delete [] exceptionSymbol.name;
        }
        void *stack[maxStackFrames];
        fputs("\nStack:\n", stderr);
        const unsigned frameCount = CaptureStackBackTrace(0, DWORD(maxStackFrames), stack, NULL);
        for (unsigned f = 0; f < frameCount; ++f)     {
            DebugSymbolResolver::Symbol symbol = resolver.resolveSymbol(DWORD64(stack[f]));
            if (symbol.name) {
                fprintf(stderr, "#%3u: %s() - 0x%p\n", f + 1, symbol.name, (const void *)symbol.address);
                delete [] symbol.name;
            } else {
                fprintf(stderr, "#%3u: Unable to obtain symbol\n", f + 1);
            }
        }
    }

    fputc('\n', stderr);

    return EXCEPTION_EXECUTE_HANDLER;
}
#elif defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
bool FatalSignalHandler::pauseOnCrash = false;

FatalSignalHandler::FatalSignalHandler()
{
    pauseOnCrash = qEnvironmentVariableIsSet("QTEST_PAUSE_ON_CRASH");
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    oldActions().fill(act);

    // Remove the handler after it is invoked.
    act.sa_flags = SA_RESETHAND | setupAlternateStack();

#  ifdef SA_SIGINFO
    act.sa_flags |= SA_SIGINFO;
    act.sa_sigaction = FatalSignalHandler::actionHandler;
#  else
    act.sa_handler = FatalSignalHandler::regularHandler;
#  endif

    // Block all fatal signals in our signal handler so we don't try to close
    // the testlog twice.
    sigemptyset(&act.sa_mask);
    for (int signal : fatalSignals)
        sigaddset(&act.sa_mask, signal);

    for (size_t i = 0; i < fatalSignals.size(); ++i)
        sigaction(fatalSignals[i], &act, &oldActions()[i]);
}

FatalSignalHandler::~FatalSignalHandler()
{
    // Restore the default signal handlers in place of ours.
    // If ours has been replaced, leave the replacement alone.
    auto isOurs = [](const struct sigaction &old) {
#  ifdef SA_SIGINFO
        return (old.sa_flags & SA_SIGINFO) && old.sa_sigaction == FatalSignalHandler::actionHandler;
#  else
        return old.sa_handler == FatalSignalHandler::regularHandler;
#  endif
    };
    struct sigaction action;

    for (size_t i = 0; i < fatalSignals.size(); ++i) {
        struct sigaction &act = oldActions()[i];
        if (act.sa_flags == 0 && act.sa_handler == SIG_DFL)
            continue; // Already the default
        if (sigaction(fatalSignals[i], nullptr, &action))
            continue; // Failed to query present handler
        if (isOurs(action))
            sigaction(fatalSignals[i], &act, nullptr);
    }

    freeAlternateStack();
}

FatalSignalHandler::OldActionsArray &FatalSignalHandler::oldActions()
{
    Q_CONSTINIT static OldActionsArray oldActions {};
    return oldActions;
}

auto FatalSignalHandler::alternateStackSize()
{
    struct R { size_t size, pageSize; };
    static constexpr size_t MinStackSize = 32 * 1024;
    size_t pageSize = sysconf(_SC_PAGESIZE);
    size_t size = SIGSTKSZ;
    if (size < MinStackSize) {
        size = MinStackSize;
    } else {
        // round up to a page
        size = (size + pageSize - 1) & -pageSize;
    }

    return R{ size + pageSize, pageSize };
}

int FatalSignalHandler::setupAlternateStack()
{
    // tvOS/watchOS both define SA_ONSTACK (in sys/signal.h) but mark sigaltstack() as
    // unavailable (__WATCHOS_PROHIBITED __TVOS_PROHIBITED in signal.h)
#  if defined(SA_ONSTACK) && !defined(Q_OS_TVOS) && !defined(Q_OS_WATCHOS)
    // Let the signal handlers use an alternate stack
    // This is necessary if SIGSEGV is to catch a stack overflow
    auto r = alternateStackSize();
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#    ifdef MAP_STACK
    flags |= MAP_STACK;
#    endif
    alternateStackBase = mmap(nullptr, r.size, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (alternateStackBase == MAP_FAILED)
        return 0;

    // mark the bottom page inaccessible, to catch a handler stack overflow
    (void) mprotect(alternateStackBase, r.pageSize, PROT_NONE);

    stack_t stack;
    stack.ss_flags = 0;
    stack.ss_size = r.size - r.pageSize;
    stack.ss_sp = static_cast<char *>(alternateStackBase) + r.pageSize;
    sigaltstack(&stack, nullptr);
    return SA_ONSTACK;
#  else
    return 0;
#  endif
}

void FatalSignalHandler::freeAlternateStack()
{
#  if defined(SA_ONSTACK) && !defined(Q_OS_TVOS) && !defined(Q_OS_WATCHOS)
    if (alternateStackBase != MAP_FAILED) {
        stack_t stack = {};
        stack.ss_flags = SS_DISABLE;
        sigaltstack(&stack, nullptr);
        munmap(alternateStackBase, alternateStackSize().size);
    }
#  endif
}

void FatalSignalHandler::actionHandler(int signum, siginfo_t *info, void *)
{
    writeToStderr("Received signal ", asyncSafeToString(signum),
                  " (SIG", signalName(signum), ")");

    bool isCrashingSignal =
            std::find(crashingSignals.begin(), crashingSignals.end(), signum) != crashingSignals.end();
    if (isCrashingSignal && (!info || info->si_code <= 0))
        isCrashingSignal = false;       // wasn't sent by the kernel, so it's not really a crash
    if (isCrashingSignal)
        printCrashingSignalInfo(info);
    else if (info && (info->si_code == SI_USER || info->si_code == SI_QUEUE))
        printSentSignalInfo(info);

    printTestRunTime();
    if (signum != SIGINT) {
        generateStackTrace();
        if (pauseOnCrash) {
            writeToStderr("Pausing process ", asyncSafeToString(getpid()),
                   " for debugging\n");
            raise(SIGSTOP);
        }
    }

    // chain back to the previous handler, if any
    for (size_t i = 0; i < fatalSignals.size(); ++i) {
        struct sigaction &act = oldActions()[i];
        if (signum != fatalSignals[i])
            continue;

        // restore the handler (if SA_RESETHAND hasn't done the job for us)
        if (SA_RESETHAND == 0 || act.sa_handler != SIG_DFL || act.sa_flags)
            (void) sigaction(signum, &act, nullptr);

        if (!isCrashingSignal)
            raise(signum);

        // signal is blocked, so it'll be delivered when we return
        return;
    }

    // we shouldn't reach here!
    std::abort();
}
#endif // defined(Q_OS_UNIX) && !defined(Q_OS_WASM)

} // namespace CrashHandler
} // namespace QTest

QT_END_NAMESPACE
