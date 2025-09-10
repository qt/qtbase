// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QTESTCRASHHANDLER_H
#define QTESTCRASHHANDLER_H

#include <QtCore/qnamespace.h>
#include <QtTest/qttestglobal.h>

#include <QtCore/private/qtools_p.h>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#endif

#ifdef Q_OS_WIN
#include <iostream>
# if !defined(Q_CC_MINGW) || (defined(Q_CC_MINGW) && defined(__MINGW64_VERSION_MAJOR))
#  include <crtdbg.h>
# endif
#include <qt_windows.h> // for Sleep
#endif

QT_BEGIN_NAMESPACE
namespace QTest {
namespace CrashHandler {
#if defined(Q_OS_UNIX) && (!defined(Q_OS_WASM) || QT_CONFIG(thread))
    struct iovec IoVec(struct iovec vec);
    struct iovec IoVec(const char *str);

    template <typename... Args> static ssize_t writeToStderr(Args &&... args)
    {
        struct iovec vec[] = { IoVec(std::forward<Args>(args))... };
        return ::writev(STDERR_FILENO, vec, std::size(vec));
    }

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

    struct iovec asyncSafeToString(int n, AsyncSafeIntBuffer &&result = Qt::Uninitialized);
#elif defined(Q_OS_WIN)
    // Windows doesn't need to be async-safe
    template <typename... Args> static void writeToStderr(Args &&... args)
    {
        (std::cerr << ... << args);
    }

    inline std::string asyncSafeToString(int n)
    {
        return std::to_string(n);
    }
#endif // defined(Q_OS_UNIX) && (!defined(Q_OS_WASM) || QT_CONFIG(thread))

    bool alreadyDebugging();
    void blockUnixSignals();

#if !defined(Q_OS_WASM) || QT_CONFIG(thread)
    void printTestRunTime();
    void generateStackTrace();
#endif

    void maybeDisableCoreDump();
    Q_TESTLIB_EXPORT void prepareStackTrace();

#if defined(Q_OS_WIN)
    // Helper class for resolving symbol names by dynamically loading "dbghelp.dll".
    class DebugSymbolResolver
    {
        Q_DISABLE_COPY_MOVE(DebugSymbolResolver)
    public:
        struct Symbol
        {
            Symbol() : name(nullptr), address(0) {}

            const char *name; // Must be freed by caller.
            DWORD64 address;
        };

        explicit DebugSymbolResolver(HANDLE process);
        ~DebugSymbolResolver() { cleanup(); }

        bool isValid() const { return m_symFromAddr; }

        Symbol resolveSymbol(DWORD64 address) const;

    private:
        // typedefs from DbgHelp.h/.dll
        struct DBGHELP_SYMBOL_INFO { // SYMBOL_INFO
            ULONG       SizeOfStruct;
            ULONG       TypeIndex;        // Type Index of symbol
            ULONG64     Reserved[2];
            ULONG       Index;
            ULONG       Size;
            ULONG64     ModBase;          // Base Address of module comtaining this symbol
            ULONG       Flags;
            ULONG64     Value;            // Value of symbol, ValuePresent should be 1
            ULONG64     Address;          // Address of symbol including base address of module
            ULONG       Register;         // register holding value or pointer to value
            ULONG       Scope;            // scope of the symbol
            ULONG       Tag;              // pdb classification
            ULONG       NameLen;          // Actual length of name
            ULONG       MaxNameLen;
            CHAR        Name[1];          // Name of symbol
        };

        typedef BOOL (__stdcall *SymInitializeType)(HANDLE, PCSTR, BOOL);
        typedef BOOL (__stdcall *SymFromAddrType)(HANDLE, DWORD64, PDWORD64, DBGHELP_SYMBOL_INFO *);

        void cleanup();

        const HANDLE m_process;
        HMODULE m_dbgHelpLib;
        SymFromAddrType m_symFromAddr;
    };

    class Q_TESTLIB_EXPORT WindowsFaultHandler
    {
    public:
        WindowsFaultHandler();

    private:
        static LONG WINAPI windowsFaultHandler(struct _EXCEPTION_POINTERS *exInfo);
    };
    using FatalSignalHandler = WindowsFaultHandler;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_WASM)
    class Q_TESTLIB_EXPORT FatalSignalHandler
    {
    public:
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

        FatalSignalHandler();
        ~FatalSignalHandler();

    private:
        Q_DISABLE_COPY_MOVE(FatalSignalHandler)

        static OldActionsArray &oldActions();
        auto alternateStackSize();
        int setupAlternateStack();
        void freeAlternateStack();

        template <typename T> static
                std::enable_if_t<sizeof(std::declval<T>().si_pid) + sizeof(std::declval<T>().si_uid) >= 1>
                printSentSignalInfo(T *info)
        {
            writeToStderr(" sent by PID ", asyncSafeToString(info->si_pid),
                          " UID ", asyncSafeToString(info->si_uid));
        }
        static void printSentSignalInfo(...) {}

        template <typename T> static
                std::enable_if_t<sizeof(std::declval<T>().si_addr) >= 1> printCrashingSignalInfo(T *info)
        {
            using HexString = std::array<char, sizeof(quintptr) * 2>;
            auto toHexString = [](quintptr u, HexString &&r = {}) {
                int shift = sizeof(quintptr) * 8 - 4;
                for (size_t i = 0; i < sizeof(quintptr) * 2; ++i, shift -= 4)
                    r[i] = QtMiscUtils::toHexLower(u >> shift);
                struct iovec vec;
                vec.iov_base = r.data();
                vec.iov_len = r.size();
                return vec;
            };
            writeToStderr(", code ", asyncSafeToString(info->si_code),
                          ", for address 0x", toHexString(quintptr(info->si_addr)));
        }
        static void printCrashingSignalInfo(...) {}
        static void actionHandler(int signum, siginfo_t *info, void * /* ucontext */);

        [[maybe_unused]] static void regularHandler(int signum)
        {
            actionHandler(signum, nullptr, nullptr);
        }

        void *alternateStackBase = MAP_FAILED;
        static bool pauseOnCrash;
    };
#else // Q_OS_WASM or weird systems
class Q_TESTLIB_EXPORT FatalSignalHandler {};
inline void blockUnixSignals() {}
#endif // Q_OS_* choice
} // namespace CrashHandler
} // namespace QTest
QT_END_NAMESPACE

#endif // QTESTCRASHHANDLER_H
