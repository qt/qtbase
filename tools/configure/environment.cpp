/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "environment.h"

#include <qdebug.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstandardpaths.h>
#include <qtemporaryfile.h>

#include <process.h>
#include <errno.h>
#include <iostream>

//#define CONFIGURE_DEBUG_EXECUTE
//#define CONFIGURE_DEBUG_CP_DIR

using namespace std;

#ifdef Q_OS_WIN32
#include <qt_windows.h>
#endif

#include <windows/registry_p.h> // from tools/shared

QT_BEGIN_NAMESPACE

struct CompilerInfo{
    Compiler compiler;
    const char *compilerStr;
    const char *regKey;
    const char *executable;
} compiler_info[] = {
    // The compilers here are sorted in a reversed-preferred order
    {CC_MINGW,   "MinGW (Minimalist GNU for Windows)",                             0, "g++.exe"},
    {CC_INTEL,   "Intel(R) C++ Compiler for 32-bit applications",                  0, "icl.exe"}, // xilink.exe, xilink5.exe, xilink6.exe, xilib.exe
    {CC_MSVC2012, "Microsoft (R) Visual Studio 2012 C/C++ Compiler (11.0)",        "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\11.0", "cl.exe"}, // link.exe, lib.exe
    {CC_MSVC2013, "Microsoft (R) Visual Studio 2013 C/C++ Compiler (12.0)",        "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\12.0", "cl.exe"}, // link.exe, lib.exe
    // Microsoft skipped version 13
    {CC_MSVC2015, "Microsoft (R) Visual Studio 2015 C/C++ Compiler (14.0)",        "Software\\Microsoft\\VisualStudio\\SxS\\VS7\\14.0", "cl.exe"}, // link.exe, lib.exe
    {CC_MSVC2017, "Microsoft (R) Visual Studio 2017 C/C++ Compiler (15.0)",        "Software\\Microsoft\\VisualStudio\\SxS\\VS7\\15.0", "cl.exe"}, // link.exe, lib.exe
    {CC_UNKNOWN, "Unknown", 0, 0},
};


// Initialize static variables
Compiler Environment::detectedCompiler = CC_UNKNOWN;

/*!
    Returns the pointer to the CompilerInfo for a \a compiler.
*/
CompilerInfo *Environment::compilerInfo(Compiler compiler)
{
    int i = 0;
    while(compiler_info[i].compiler != compiler && compiler_info[i].compiler != CC_UNKNOWN)
        ++i;
    return &(compiler_info[i]);
}

/*!
    Returns the qmakespec for the compiler detected on the system.
*/
QString Environment::detectQMakeSpec()
{
    QString spec;
    switch (detectCompiler()) {
    case CC_MSVC2017:
        spec = "win32-msvc2017";
        break;
    case CC_MSVC2015:
        spec = "win32-msvc2015";
        break;
    case CC_MSVC2013:
        spec = "win32-msvc2013";
        break;
    case CC_MSVC2012:
        spec = "win32-msvc2012";
        break;
    case CC_INTEL:
        spec = "win32-icc";
        break;
    case CC_MINGW:
        spec = "win32-g++";
        break;
    default:
        break;
    }

    return spec;
}

/*!
    Returns the enum of the compiler which was detected on the system.
    The compilers are detected in the order as entered into the
    compiler_info list.

    If more than one compiler is found, CC_UNKNOWN is returned.
*/
Compiler Environment::detectCompiler()
{
#ifndef Q_OS_WIN32
    return CC_UNKNOWN; // Always generate CC_UNKNOWN on other platforms
#else
    if(detectedCompiler != CC_UNKNOWN)
        return detectedCompiler;

    int installed = 0;

    // Check for compilers in registry first, to see which version is in PATH
    QString paths = qgetenv("PATH");
    QStringList pathlist = paths.toLower().split(";");
    for(int i = 0; compiler_info[i].compiler; ++i) {
        QString productPath = qt_readRegistryKey(HKEY_LOCAL_MACHINE, compiler_info[i].regKey,
                                                 KEY_WOW64_32KEY).toLower();
        if (productPath.length()) {
            QStringList::iterator it;
            for(it = pathlist.begin(); it != pathlist.end(); ++it) {
                if((*it).contains(productPath)) {
                    if (detectedCompiler != compiler_info[i].compiler) {
                        ++installed;
                        detectedCompiler = compiler_info[i].compiler;
                    }
                    /* else {

                        We detected the same compiler again, which happens when
                        configure is build with the 64-bit compiler. Skip the
                        duplicate so that we don't think it's installed twice.

                    }
                    */
                    break;
                }
            }
        }
    }

    // Now just go looking for the executables, and accept any executable as the lowest version
    if (!installed) {
        for(int i = 0; compiler_info[i].compiler; ++i) {
            QString executable = QString(compiler_info[i].executable).toLower();
            if (executable.length() && !QStandardPaths::findExecutable(executable).isEmpty()) {
                if (detectedCompiler != compiler_info[i].compiler) {
                    ++installed;
                    detectedCompiler = compiler_info[i].compiler;
                }
                /* else {

                    We detected the same compiler again, which happens when
                    configure is build with the 64-bit compiler. Skip the
                    duplicate so that we don't think it's installed twice.

                }
                */
                break;
            }
        }
    }

    if (installed > 1) {
        cout << "Found more than one known compiler! Using \"" << compilerInfo(detectedCompiler)->compilerStr << "\"" << endl;
        detectedCompiler = CC_UNKNOWN;
    }
    return detectedCompiler;
#endif
};

/*!
    Creates a commandling from \a program and it \a arguments,
    escaping characters that needs it.
*/
static QString qt_create_commandline(const QString &program, const QStringList &arguments)
{
    QString programName = program;
    if (!programName.startsWith("\"") && !programName.endsWith("\"") && programName.contains(" "))
        programName = "\"" + programName + "\"";
    programName.replace("/", "\\");

    QString args;
    // add the prgram as the first arrg ... it works better
    args = programName + " ";
    for (int i=0; i<arguments.size(); ++i) {
        QString tmp = arguments.at(i);
        // in the case of \" already being in the string the \ must also be escaped
        tmp.replace( "\\\"", "\\\\\"" );
        // escape a single " because the arguments will be parsed
        tmp.replace( "\"", "\\\"" );
        if (tmp.isEmpty() || tmp.contains(' ') || tmp.contains('\t')) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            QString endQuote("\"");
            int i = tmp.length();
            while (i>0 && tmp.at(i-1) == '\\') {
                --i;
                endQuote += "\\";
            }
            args += QString(" \"") + tmp.left(i) + endQuote;
        } else {
            args += ' ' + tmp;
        }
    }
    return args;
}

/*!
    Creates a QByteArray of the \a environment.
*/
static QByteArray qt_create_environment(const QStringList &environment)
{
    QByteArray envlist;
    if (environment.isEmpty())
        return envlist;

    int pos = 0;
    // add PATH if necessary (for DLL loading)
    QByteArray path = qgetenv("PATH");
    if (environment.filter(QRegExp("^PATH=",Qt::CaseInsensitive)).isEmpty() && !path.isNull()) {
            QString tmp = QString(QLatin1String("PATH=%1")).arg(QString::fromLocal8Bit(path));
            uint tmpSize = sizeof(wchar_t) * (tmp.length() + 1);
            envlist.resize(envlist.size() + tmpSize);
            memcpy(envlist.data() + pos, tmp.utf16(), tmpSize);
            pos += tmpSize;
    }
    // add the user environment
    foreach (const QString &tmp, environment) {
            uint tmpSize = sizeof(wchar_t) * (tmp.length() + 1);
            envlist.resize(envlist.size() + tmpSize);
            memcpy(envlist.data() + pos, tmp.utf16(), tmpSize);
            pos += tmpSize;
    }
    // add the 2 terminating 0 (actually 4, just to be on the safe side)
    envlist.resize(envlist.size() + 4);
    envlist[pos++] = 0;
    envlist[pos++] = 0;
    envlist[pos++] = 0;
    envlist[pos++] = 0;

    return envlist;
}

/*!
    Executes the command described in \a arguments, in the
    environment inherited from the parent process, with the
    \a additionalEnv settings applied.
    \a removeEnv removes the specified environment variables from
    the environment of the executed process.

    Returns the exit value of the process, or -1 if the command could
    not be executed.

    This function uses _(w)spawnvpe to spawn a process by searching
    through the PATH environment variable.
*/
int Environment::execute(QStringList arguments, const QStringList &additionalEnv, const QStringList &removeEnv)
{
#ifdef CONFIGURE_DEBUG_EXECUTE
    qDebug() << "About to Execute: " << arguments;
    qDebug() << "   " << QDir::currentPath();
    qDebug() << "   " << additionalEnv;
    qDebug() << "   " << removeEnv;
#endif
    // Create the full environment from the current environment and
    // the additionalEnv strings, then remove all variables defined
    // in removeEnv
    QMap<QString, QString> fullEnvMap;
    LPWSTR envStrings = GetEnvironmentStrings();
    if (envStrings) {
        int strLen = 0;
        for (LPWSTR envString = envStrings; *(envString); envString += strLen + 1) {
            strLen = int(wcslen(envString));
            QString str = QString((const QChar*)envString, strLen);
            if (!str.startsWith("=")) { // These are added by the system
                int sepIndex = str.indexOf('=');
                fullEnvMap.insert(str.left(sepIndex).toUpper(), str.mid(sepIndex +1));
            }
        }
    }
    FreeEnvironmentStrings(envStrings);

    // Add additionalEnv variables
    for (int i = 0; i < additionalEnv.count(); ++i) {
        const QString &str = additionalEnv.at(i);
        int sepIndex = str.indexOf('=');
        fullEnvMap.insert(str.left(sepIndex).toUpper(), str.mid(sepIndex +1));
    }

    // Remove removeEnv variables
    for (int j = 0; j < removeEnv.count(); ++j)
        fullEnvMap.remove(removeEnv.at(j).toUpper());

    // Add all variables to a QStringList
    QStringList fullEnv;
    QMapIterator<QString, QString> it(fullEnvMap);
    while (it.hasNext()) {
        it.next();
        fullEnv += QString(it.key() + "=" + it.value());
    }

    // ----------------------------
    QString program = arguments.takeAt(0);
    QString args = qt_create_commandline(program, arguments);
    QByteArray envlist = qt_create_environment(fullEnv);

    DWORD exitCode = DWORD(-1);
    PROCESS_INFORMATION procInfo;
    memset(&procInfo, 0, sizeof(procInfo));

    STARTUPINFO startInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);

    bool couldExecute = CreateProcess(0, (wchar_t*)args.utf16(),
                                      0, 0, true, CREATE_UNICODE_ENVIRONMENT,
                                      envlist.isEmpty() ? 0 : envlist.data(),
                                      0, &startInfo, &procInfo);

    if (couldExecute) {
        WaitForSingleObject(procInfo.hProcess, INFINITE);
        GetExitCodeProcess(procInfo.hProcess, &exitCode);
        CloseHandle(procInfo.hThread);
        CloseHandle(procInfo.hProcess);
    }


    if (exitCode == DWORD(-1)) {
        switch(GetLastError()) {
        case E2BIG:
            cerr << "execute: Argument list exceeds 1024 bytes" << endl;
            foreach (const QString &arg, arguments)
                cerr << "   (" << arg.toLocal8Bit().constData() << ")" << endl;
            break;
        case ENOENT:
            cerr << "execute: File or path is not found (" << program.toLocal8Bit().constData() << ")" << endl;
            break;
        case ENOEXEC:
            cerr << "execute: Specified file is not executable or has invalid executable-file format (" << program.toLocal8Bit().constData() << ")" << endl;
            break;
        case ENOMEM:
            cerr << "execute: Not enough memory is available to execute new process." << endl;
            break;
        default:
            cerr << "execute: Unknown error" << endl;
            foreach (const QString &arg, arguments)
                cerr << "   (" << arg.toLocal8Bit().constData() << ")" << endl;
            break;
        }
    }
    return exitCode;
}

/*!
    Executes \a command with _popen() and returns the stdout of the command.

    Taken from qmake's system() command.
*/
QString Environment::execute(const QString &command, int *returnCode)
{
    QString output;
    FILE *proc = _popen(command.toLatin1().constData(), "r");
    char buff[256];
    while (proc && !feof(proc)) {
        int read_in = int(fread(buff, 1, 255, proc));
        if (!read_in)
            break;
        buff[read_in] = '\0';
        output += buff;
    }
    if (proc) {
        int r = _pclose(proc);
        if (returnCode)
            *returnCode = r;
    }
    return output;
}

QT_END_NAMESPACE
