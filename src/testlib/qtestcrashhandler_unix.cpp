// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qtestcrashhandler_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qcore_unix_p.h>

#include <QtTest/qtestcase.h>
#include <QtTest/private/qtestlog_p.h>

#if !defined(Q_OS_INTEGRITY) || __GHS_VERSION_NUMBER > 202014
#  include <charconv>
#else
// Broken implementation, causes link failures just by #include'ing!
#  undef __cpp_lib_to_chars     // in case <version> was included
#endif
#include <string_view>

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#if __has_include(<paths.h>)
# include <paths.h>
#endif
#include <signal.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>
# if !defined(Q_OS_INTEGRITY)
#  include <sys/resource.h>
# endif
# if __has_include(<sys/ucontext.h>)
#  include <sys/ucontext.h>
# elif __has_include(<ucontext.h>)
#  include <ucontext.h>
# else
using ucontext_t = void;
# endif

#if defined(Q_OS_MACOS)
#include <QtCore/private/qcore_mac_p.h>
#include <QtTest/private/qtestutil_macos_p.h>

#include <IOKit/pwr_mgt/IOPMLib.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <CoreFoundation/CFPreferences.h>

#define CSR_ALLOW_UNRESTRICTED_FS (1 << 1)
#endif

#if defined(Q_OS_LINUX)
#include <sys/prctl.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#if defined(Q_OS_WASM)
#include <emscripten.h>
#endif

#ifndef _PATH_DEFPATH
#  define _PATH_DEFPATH     "/usr/bin:/bin"
#endif
#ifndef SIGSTKSZ
#  define SIGSTKSZ          0       /* we have code to set the minimum */
#endif
#ifndef SA_RESETHAND
#  define SA_RESETHAND      0
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#  define OUR_SIGNALS(F)    \
            F(HUP)              \
            F(INT)              \
            F(QUIT)             \
            F(ABRT)             \
            F(ILL)              \
            F(BUS)              \
            F(FPE)              \
            F(SEGV)             \
            F(PIPE)             \
            F(TERM)             \
    /**/
#  define CASE_LABEL(S)             case SIG ## S:  return QT_STRINGIFY(S);
#  define ENUMERATE_SIGNALS(S)      SIG ## S,
static const char *signalName(int signum) noexcept
{
    switch (signum) {
    OUR_SIGNALS(CASE_LABEL)
    }

#  if defined(__GLIBC_MINOR__) && (__GLIBC_MINOR__ >= 32 || __GLIBC__ > 2)
    // get the other signal names from glibc 2.32
    // (accessing the sys_sigabbrev variable causes linker warnings)
    if (const char *p = sigabbrev_np(signum))
        return p;
#  endif
    return "???";
}
static constexpr std::array fatalSignals = {
    OUR_SIGNALS(ENUMERATE_SIGNALS)
};
#  undef CASE_LABEL
#  undef ENUMERATE_SIGNALS

static constexpr std::array crashingSignals = {
    // Crash signals are special, because if we return from the handler
    // without adjusting the machine state, the same instruction that
    // originally caused the crash will get re-executed and will thus cause
    // the same crash again. This is useful if our parent process logs the
    // exit result or if core dumps are enabled: the core file will point
    // to the actual instruction that crashed.
    SIGILL, SIGBUS, SIGFPE, SIGSEGV
};
using OldActionsArray = std::array<struct sigaction, fatalSignals.size()>;

template <typename... Args> static ssize_t writeToStderr(Args &&... args)
{
    auto makeIovec = [](std::string_view arg) {
        struct iovec r = {};
        r.iov_base = const_cast<char *>(arg.data());
        r.iov_len = arg.size();
        return r;
    };
    struct iovec vec[] = { makeIovec(std::forward<Args>(args))... };
    return ::writev(STDERR_FILENO, vec, std::size(vec));
}

namespace {
// async-signal-safe conversion from int to string
struct AsyncSafeIntBuffer
{
    // digits10 + 1 for all possible digits
    // +1 for the sign
    // +1 for the terminating null
    static constexpr int Digits10 = std::numeric_limits<int>::digits10 + 3;
    std::array<char, Digits10> array;
    constexpr AsyncSafeIntBuffer() : array{} {}     // initializes array
    AsyncSafeIntBuffer(Qt::Initialization) {}       // leaves array uninitialized
};

std::string_view asyncSafeToString(int n, AsyncSafeIntBuffer &&result = Qt::Uninitialized)
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
    return std::string_view(result.array.data(), ptr - result.array.data());
};

std::string_view asyncSafeToHexString(quintptr u, char *ptr)
{
    // We format with leading zeroes so the output is of fixed length.
    // Formatting to shorter is more complex and unnecessary here (unlike
    // decimals above).
    int shift = sizeof(quintptr) * 8 - 4;
    ptr[0] = '0';
    ptr[1] = 'x';
    for (size_t i = 0; i < sizeof(quintptr) * 2; ++i, shift -= 4)
        ptr[i + 2] = QtMiscUtils::toHexLower(u >> shift);

    return std::string_view(ptr, sizeof(quintptr) * 2 + 2);
}
} // unnamed namespace

namespace QTest {
namespace CrashHandler {
Q_CONSTINIT static OldActionsArray oldActions {};
static bool pauseOnCrash = false;

static void actionHandler(int signum, siginfo_t *info, void * /* ucontext */);

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

static bool hasSystemCrashReporter()
{
#if defined(Q_OS_MACOS)
    return QTestPrivate::macCrashReporterWillShowDialog();
#else
    return false;
#endif
}

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
    // Try to handle https://github.com/llvm/llvm-project/issues/53254,
    // where LLDB will hang and fail to provide a valid stack trace.
# if defined(Q_PROCESSOR_ARM)
    return;
# else
    std::optional<uint32_t> sipConfiguration = qt_mac_sipConfiguration();
    if (!sipConfiguration || !(*sipConfiguration & CSR_ALLOW_UNRESTRICTED_FS))
        return;
# endif
#endif

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
}

void printTestRunTime()
{
    const int msecsFunctionTime = qRound(QTestLog::msecsFunctionTime());
    const int msecsTotalTime = qRound(QTestLog::msecsTotalTime());
    const char *const name = QTest::currentTestFunction();
    writeToStderr("\n         ", name ? name : "[Non-test]",
                  " function time: ", asyncSafeToString(msecsFunctionTime),
                  "ms, total time: ", asyncSafeToString(msecsTotalTime), "ms\n");
}

static quintptr getProgramCounter(void *ucontext)
{
    quintptr pc = 0;
    if ([[maybe_unused]] auto ctx = static_cast<ucontext_t *>(ucontext)) {
#if 0 // keep the list below alphabetical

#elif defined(Q_OS_DARWIN) && defined(Q_PROCESSOR_ARM_64)
        pc = arm_thread_state64_get_pc(ctx->uc_mcontext->__ss);
#elif defined(Q_OS_DARWIN) && defined(Q_PROCESSOR_X86_64)
        pc = ctx->uc_mcontext->__ss.__rip;

#elif defined(Q_OS_FREEBSD) && defined(Q_PROCESSOR_X86_32)
        pc = ctx->uc_mcontext.mc_eip;
#elif defined(Q_OS_FREEBSD) && defined(Q_PROCESSOR_X86_64)
        pc = ctx->uc_mcontext.mc_rip;

#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_ARM_32)
        // pc = ctx->uc_mcontext.arm_pc;                // untested
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_ARM_64)
        pc = ctx->uc_mcontext.pc;
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_MIPS)
        // pc = ctx->uc_mcontext.pc;                    // untested
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_LOONGARCH)
        // pc = ctx->uc_mcontext.__pc;                  // untested
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_POWER_32)
        // pc = ctx->uc_mcontext.uc_regs->gregs[PT_NIP]; // untested
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_POWER_64)
        // pc = ctx->uc_mcontext.gregs[PT_NIP];         // untested
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_RISCV)
        // pc = ctx->uc_mcontext.__gregs[REG_PC];       // untested
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_32)
        pc = ctx->uc_mcontext.gregs[REG_EIP];
#elif defined(Q_OS_LINUX) && defined(Q_PROCESSOR_X86_64)
        pc = ctx->uc_mcontext.gregs[REG_RIP];
#endif
    }
    return pc;
}

void generateStackTrace(quintptr ip)
{
    if (debugger == None || alreadyDebugging())
        return;

#  if defined(Q_OS_LINUX) && defined(PR_SET_PTRACER)
    // allow ourselves to be debugged
    (void) prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY);
#  endif

#  if !defined(Q_OS_INTEGRITY) && !defined(Q_OS_VXWORKS)
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

        // disassemble the crashing instruction, if known
        // (syntax is the same for gdb and lldb)
        char disasmInstr[sizeof("x/i 0x") + sizeof(ip) * 2] = {};   // zero-init for terminator
        if (ip) {
            strcpy(disasmInstr, "x/i ");
            asyncSafeToHexString(ip, disasmInstr + strlen(disasmInstr));
        }

        struct Args {
            std::array<const char *, 16> argv;
            int count = 0;
            Args &operator<<(const char *arg)
            {
                Q_ASSERT(count < int(argv.size()));
                argv[count++] = arg;
                return *this;
            }
            operator char **() const { return const_cast<char **>(argv.data()); }
        } argv;

        switch (debugger) {
        case None:
            Q_UNREACHABLE();
            break;
        case Gdb:
            argv << "gdb" << "--nx" << "--batch";
            if (ip)
                argv << "-ex" << disasmInstr;
            argv << "-ex" << "thread apply all bt"
                 << "-ex" << "printf \"\\n\""
                 << "-ex" << "info proc mappings"
                 << "--pid";
            break;
        case Lldb:
            argv << "lldb" << "--no-lldbinit" << "--batch";
            if (ip)
                argv << "-o" << disasmInstr;
            argv << "-o" << "bt all"
                 << "--attach-pid";
            break;
        }
        if (argv.count) {
            argv << pidbuffer.array.data() << nullptr;
            execvp(argv.argv[0], argv);
        }
        _exit(1);
    } else if (pid < 0) {
        writeToStderr("Failed to start debugger.\n");
    } else {
        int ret;
        QT_EINTR_LOOP(ret, waitpid(pid, nullptr, 0));
    }

    writeToStderr("=== End of stack trace ===\n");
#  else
    Q_UNUSED(ip);
#  endif // !Q_OS_INTEGRITY && !Q_OS_VXWORKS
}

#ifndef Q_OS_WASM // no signal handling for WASM
void blockUnixSignals()
{
    // Block most Unix signals so the WatchDog thread won't be called when
    // external signals are delivered, thus avoiding interfering with the test
    sigset_t set;
    sigfillset(&set);

    // we allow the crashing signals, in case we have bugs
    for (int signo : fatalSignals)
        sigdelset(&set, signo);

    pthread_sigmask(SIG_BLOCK, &set, nullptr);
}

static std::string_view unixSignalCodeToName(int signo, int code) noexcept
{
    switch (signo) {
    case SIGFPE:
        switch (code) {
#ifdef FPE_INTDIV
        case FPE_INTDIV: return "FPE_INTDIV";        // Integer divide by zero.
#endif
#ifdef FPE_INTOVF
        case FPE_INTOVF: return "FPE_INTOVF";        // Integer overflow.
#endif
#ifdef FPE_FLTDIV
        case FPE_FLTDIV: return "FPE_FLTDIV";        // Floating point divide by zero.
#endif
#ifdef FPE_FLTOVF
        case FPE_FLTOVF: return "FPE_FLTOVF";        // Floating point overflow.
#endif
#ifdef FPE_FLTUND
        case FPE_FLTUND: return "FPE_FLTUND";        // Floating point underflow.
#endif
#ifdef FPE_FLTRES
        case FPE_FLTRES: return "FPE_FLTRES";        // Floating point inexact result.
#endif
#ifdef FPE_FLTINV
        case FPE_FLTINV: return "FPE_FLTINV";        // Floating point invalid operation.
#endif
#ifdef FPE_FLTSUB
        case FPE_FLTSUB: return "FPE_FLTSUB";        // Subscript out of range.
#endif
#ifdef FPE_FLTUNK
        case FPE_FLTUNK: return "FPE_FLTUNK";        // Undiagnosed floating-point exception.
#endif
#ifdef FPE_CONDTRAP
        case FPE_CONDTRAP: return "FPE_CONDTRAP";    // Trap on condition.
#endif
        }
        break;

    case SIGILL:
        switch (code) {
#ifdef ILL_ILLOPC
        case ILL_ILLOPC: return "ILL_ILLOPC";        // Illegal opcode.
#endif
#ifdef ILL_ILLOPN
        case ILL_ILLOPN: return "ILL_ILLOPN";        // Illegal operand.
#endif
#ifdef ILL_ILLADR
        case ILL_ILLADR: return "ILL_ILLADR";        // Illegal addressing mode.
#endif
#ifdef ILL_ILLTRP
        case ILL_ILLTRP: return "ILL_ILLTRP";        // Illegal trap.
#endif
#ifdef ILL_PRVOPC
        case ILL_PRVOPC: return "ILL_PRVOPC";        // Privileged opcode.
#endif
#ifdef ILL_PRVREG
        case ILL_PRVREG: return "ILL_PRVREG";        // Privileged register.
#endif
#ifdef ILL_COPROC
        case ILL_COPROC: return "ILL_COPROC";        // Coprocessor error.
#endif
#ifdef ILL_BADSTK
        case ILL_BADSTK: return "ILL_BADSTK";        // Internal stack error.
#endif
#ifdef ILL_BADIADDR
        case ILL_BADIADDR: return "ILL_BADIADDR";    // Unimplemented instruction address.
#endif
        }
        break;

    case SIGSEGV:
        switch (code) {
#ifdef SEGV_MAPERR
        case SEGV_MAPERR: return "SEGV_MAPERR";        // Address not mapped to object.
#endif
#ifdef SEGV_ACCERR
        case SEGV_ACCERR: return "SEGV_ACCERR";        // Invalid permissions for mapped object.
#endif
#ifdef SEGV_BNDERR
        // Intel MPX - deprecated
        case SEGV_BNDERR: return "SEGV_BNDERR";        // Bounds checking failure.
#endif
#ifdef SEGV_PKUERR
        // Intel PKRU
        case SEGV_PKUERR: return "SEGV_PKUERR";        // Protection key checking failure.
#endif
#ifdef Q_PROCESSOR_SPARC
        // these seem to be Sparc-specific on Linux
#  ifdef SEGV_ACCADI
        case SEGV_ACCADI: return "SEGV_ACCADI";        // ADI not enabled for mapped object.
#  endif
#  ifdef SEGV_ADIDERR
        case SEGV_ADIDERR: return "SEGV_ADIDERR";      // Disrupting MCD error.
#  endif
#  ifdef SEGV_ADIPERR
        case SEGV_ADIPERR: return "SEGV_ADIPERR";      // Precise MCD exception.
#  endif
#endif
#ifdef Q_PROCESSOR_ARM
#  ifdef SEGV_MTEAERR
        case SEGV_MTEAERR: return "SEGV_MTEAERR";      // Asynchronous ARM MTE error.
#  endif
#  ifdef SEGV_MTESERR
        case SEGV_MTESERR: return "SEGV_MTESERR";      // Synchronous ARM MTE exception.
#  endif
#endif
#ifdef SEGV_CPERR
        // seen on both AArch64 and x86 Linux
        case SEGV_CPERR:  return "SEGV_CPERR";         // Control protection fault
#endif
        }
        break;

    case SIGBUS:
        switch (code) {
#ifdef BUS_ADRALN
        case BUS_ADRALN: return "BUS_ADRALN";        // Invalid address alignment.
#endif
#ifdef BUS_ADRERR
        case BUS_ADRERR: return "BUS_ADRERR";        // Non-existant physical address.
#endif
#ifdef BUS_OBJERR
        case BUS_OBJERR: return "BUS_OBJERR";        // Object specific hardware error.
#endif
#ifdef BUS_MCEERR_AR
        case BUS_MCEERR_AR: return "BUS_MCEERR_AR";  // Hardware memory error: action required.
#endif
#ifdef BUS_MCEERR_AO
        case BUS_MCEERR_AO: return "BUS_MCEERR_AO";  // Hardware memory error: action optional.
#endif
        }
        break;
    }

    return {};
}

template <typename T> static
        std::enable_if_t<sizeof(std::declval<T>().si_pid) + sizeof(std::declval<T>().si_uid) >= 1>
printSentSignalInfo(T *info)
{
    writeToStderr(" sent by PID ", asyncSafeToString(info->si_pid),
                  " UID ", asyncSafeToString(info->si_uid));
}
[[maybe_unused]] static void printSentSignalInfo(...) {}

template <typename T> static std::enable_if_t<sizeof(std::declval<T>().si_addr) >= 1>
printCrashingSignalInfo(T *info, quintptr pc)
{
    using HexString = std::array<char, sizeof(quintptr) * 2 + 2>;
    auto toHexString = [](quintptr u, HexString &&r = {}) {
        return asyncSafeToHexString(u, r.data());
    };

    std::string_view name = unixSignalCodeToName(info->si_signo, info->si_code);
    writeToStderr(", code ", name.size() ? name : asyncSafeToString(info->si_code));
    if (pc)
        writeToStderr(", at instruction address ", toHexString(pc));
    writeToStderr(", accessing address ", toHexString(quintptr(info->si_addr)));
}
[[maybe_unused]] static void printCrashingSignalInfo(...) {}

[[maybe_unused]] static void regularHandler(int signum)
{
    actionHandler(signum, nullptr, nullptr);
}

FatalSignalHandler::FatalSignalHandler()
{
    pauseOnCrash = qEnvironmentVariableIsSet("QTEST_PAUSE_ON_CRASH");
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_DFL;
    oldActions.fill(act);

    // Remove the handler after it is invoked.
    act.sa_flags = SA_RESETHAND | setupAlternateStack();

#  ifdef SA_SIGINFO
    act.sa_flags |= SA_SIGINFO;
    act.sa_sigaction = actionHandler;
#  else
    act.sa_handler = regularHandler;
#  endif

    // Block all fatal signals in our signal handler so we don't try to close
    // the testlog twice.
    sigemptyset(&act.sa_mask);
    for (int signal : fatalSignals)
        sigaddset(&act.sa_mask, signal);

    for (size_t i = 0; i < fatalSignals.size(); ++i)
        sigaction(fatalSignals[i], &act, &oldActions[i]);
}

FatalSignalHandler::~FatalSignalHandler()
{
    // Restore the default signal handlers in place of ours.
    // If ours has been replaced, leave the replacement alone.
    auto isOurs = [](const struct sigaction &old) {
#  ifdef SA_SIGINFO
        return (old.sa_flags & SA_SIGINFO) && old.sa_sigaction == actionHandler;
#  else
        return old.sa_handler == regularHandler;
#  endif
    };
    struct sigaction action;

    for (size_t i = 0; i < fatalSignals.size(); ++i) {
        struct sigaction &act = oldActions[i];
        if (sigaction(fatalSignals[i], nullptr, &action))
            continue; // Failed to query present handler
        if (action.sa_flags == 0 && action.sa_handler == SIG_DFL)
            continue; // Already the default
        if (isOurs(action))
            sigaction(fatalSignals[i], &act, nullptr);
    }

    freeAlternateStack();
}

static auto alternateStackSize() noexcept
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

void actionHandler(int signum, siginfo_t *info, void *ucontext)
{
    writeToStderr("Received signal ", asyncSafeToString(signum),
                  " (SIG", signalName(signum), ")");

    quintptr pc = 0;
    bool isCrashingSignal =
            std::find(crashingSignals.begin(), crashingSignals.end(), signum) != crashingSignals.end();
    if (isCrashingSignal && (!info || info->si_code <= 0))
        isCrashingSignal = false;       // wasn't sent by the kernel, so it's not really a crash
    if (isCrashingSignal)
        printCrashingSignalInfo(info, (pc = getProgramCounter(ucontext)));
    else if (info && (info->si_code == SI_USER || info->si_code == SI_QUEUE))
        printSentSignalInfo(info);

    printTestRunTime();
    if (signum != SIGINT) {
        generateStackTrace(pc);
        if (pauseOnCrash) {
            writeToStderr("Pausing process ", asyncSafeToString(getpid()),
                   " for debugging\n");
            raise(SIGSTOP);
        }
    }

    // chain back to the previous handler, if any
    for (size_t i = 0; i < fatalSignals.size(); ++i) {
        struct sigaction &act = oldActions[i];
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
#endif // !defined(Q_OS_WASM)

} // namespace CrashHandler
} // namespace QTest

QT_END_NAMESPACE
