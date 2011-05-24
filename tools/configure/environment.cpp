/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "environment.h"

#include <process.h>
#include <iostream>
#include <qdebug.h>
#include <QDir>
#include <QStringList>
#include <QMap>
#include <QDir>
#include <QFile>
#include <QFileInfo>

//#define CONFIGURE_DEBUG_EXECUTE
//#define CONFIGURE_DEBUG_CP_DIR

using namespace std;

#ifdef Q_OS_WIN32
#include <qt_windows.h>
#endif

#include <symbian/epocroot_p.h> // from tools/shared
#include <windows/registry_p.h> // from tools/shared

QT_BEGIN_NAMESPACE

struct CompilerInfo{
    Compiler compiler;
    const char *compilerStr;
    const char *regKey;
    const char *executable;
} compiler_info[] = {
    // The compilers here are sorted in a reversed-preferred order
    {CC_BORLAND, "Borland C++",                                                    0, "bcc32.exe"},
    {CC_MINGW,   "MinGW (Minimalist GNU for Windows)",                             0, "g++.exe"},
    {CC_INTEL,   "Intel(R) C++ Compiler for 32-bit applications",                  0, "icl.exe"}, // xilink.exe, xilink5.exe, xilink6.exe, xilib.exe
    {CC_NET2003, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2003 (7.1)",  "Software\\Microsoft\\VisualStudio\\7.1\\Setup\\VC\\ProductDir", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2005, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2005 (8.0)",  "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\8.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2008, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2008 (9.0)",  "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\9.0", "cl.exe"}, // link.exe, lib.exe
    {CC_NET2010, "Microsoft (R) 32-bit C/C++ Optimizing Compiler.NET 2010 (10.0)", "Software\\Microsoft\\VisualStudio\\SxS\\VC7\\10.0", "cl.exe"}, // link.exe, lib.exe
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
    case CC_NET2010:
        spec = "win32-msvc2010";
        break;
    case CC_NET2008:
        spec = "win32-msvc2008";
        break;
    case CC_NET2005:
        spec = "win32-msvc2005";
        break;
    case CC_NET2003:
        spec = "win32-msvc2003";
        break;
    case CC_INTEL:
        spec = "win32-icc";
        break;
    case CC_MINGW:
        spec = "win32-g++";
        break;
    case CC_BORLAND:
        spec = "win32-borland";
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
        QString productPath = qt_readRegistryKey(HKEY_LOCAL_MACHINE, compiler_info[i].regKey).toLower();
        if (productPath.length()) {
            QStringList::iterator it;
            for(it = pathlist.begin(); it != pathlist.end(); ++it) {
                if((*it).contains(productPath)) {
                    ++installed;
                    detectedCompiler = compiler_info[i].compiler;
                    break;
                }
            }
        }
    }

    // Now just go looking for the executables, and accept any executable as the lowest version
    if (!installed) {
        for(int i = 0; compiler_info[i].compiler; ++i) {
            QString executable = QString(compiler_info[i].executable).toLower();
            if (executable.length() && Environment::detectExecutable(executable)) {
                ++installed;
                detectedCompiler = compiler_info[i].compiler;
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
    Returns true if the \a executable could be loaded, else false.
    This means that the executable either is in the current directory
    or in the PATH.
*/
bool Environment::detectExecutable(const QString &executable)
{
    PROCESS_INFORMATION procInfo;
    memset(&procInfo, 0, sizeof(procInfo));

    STARTUPINFO startInfo;
    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);

    bool couldExecute = CreateProcess(0, (wchar_t*)executable.utf16(),
                                      0, 0, false,
                                      CREATE_NO_WINDOW | CREATE_SUSPENDED,
                                      0, 0, &startInfo, &procInfo);

    if (couldExecute) {
        CloseHandle(procInfo.hThread);
        TerminateProcess(procInfo.hProcess, 0);
        CloseHandle(procInfo.hProcess);
    }
    return couldExecute;
}

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
            strLen = wcslen(envString);
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
    Copies the \a srcDir contents into \a destDir.

    If \a includeSrcDir is not empty, any files with 'h', 'prf', or 'conf' suffixes
    will not be copied over from \a srcDir. Instead a new file will be created
    in \a destDir with the same name and that file will include a file with the
    same name from the \a includeSrcDir using relative path and appropriate
    syntax for the file type.

    Returns true if copying was successful.
*/
bool Environment::cpdir(const QString &srcDir,
                        const QString &destDir,
                        const QString &includeSrcDir)
{
    QString cleanSrcName = QDir::cleanPath(srcDir);
    QString cleanDstName = QDir::cleanPath(destDir);
    QString cleanIncludeName = QDir::cleanPath(includeSrcDir);

#ifdef CONFIGURE_DEBUG_CP_DIR
    qDebug() << "Attempt to cpdir " << cleanSrcName << "->" << cleanDstName;
#endif
    if(!QFile::exists(cleanDstName) && !QDir().mkpath(cleanDstName)) {
	qDebug() << "cpdir: Failure to create " << cleanDstName;
	return false;
    }

    bool result = true;
    QDir dir = QDir(cleanSrcName);
    QDir destinationDir = QDir(cleanDstName);
    QFileInfoList allEntries = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; result && (i < allEntries.count()); ++i) {
        QFileInfo entry = allEntries.at(i);
	bool intermediate = true;
        if (entry.isDir()) {
            QString newIncSrcDir;
            if (!includeSrcDir.isEmpty())
                newIncSrcDir = QString("%1/%2").arg(cleanIncludeName).arg(entry.fileName());

            intermediate = cpdir(QString("%1/%2").arg(cleanSrcName).arg(entry.fileName()),
                                 QString("%1/%2").arg(cleanDstName).arg(entry.fileName()),
                                 newIncSrcDir);
        } else {
            QString destFile = QString("%1/%2").arg(cleanDstName).arg(entry.fileName());
#ifdef CONFIGURE_DEBUG_CP_DIR
	    qDebug() << "About to cp (file)" << entry.absoluteFilePath() << "->" << destFile;
#endif
	    QFile::remove(destFile);
            QString suffix = entry.suffix();
            if (!includeSrcDir.isEmpty() && (suffix == "prf" || suffix == "conf" || suffix == "h")) {
                QString relativeIncludeFilePath = QString("%1/%2").arg(cleanIncludeName).arg(entry.fileName());
                relativeIncludeFilePath = destinationDir.relativeFilePath(relativeIncludeFilePath);
#ifdef CONFIGURE_DEBUG_CP_DIR
                qDebug() << "...instead generate relative include to" << relativeIncludeFilePath;
#endif
                QFile currentFile(destFile);
                if (currentFile.open(QFile::WriteOnly | QFile::Text)) {
                    QTextStream fileStream;
                    fileStream.setDevice(&currentFile);

                    if (suffix == "prf" || suffix == "conf") {
                        if (entry.fileName() == "qmake.conf") {
                            // While QMAKESPEC_ORIGINAL being relative or absolute doesn't matter for the
                            // primary use of this variable by qmake to identify the original mkspec, the
                            // variable is also used for few special cases where the absolute path is required.
                            // Conversely, the include of the original qmake.conf must be done using relative path,
                            // as some Qt binary deployments are done in a manner that doesn't allow for patching
                            // the paths at the installation time.
                            fileStream << "QMAKESPEC_ORIGINAL=" << cleanSrcName << endl << endl;
                        }
                        fileStream << "include(" << relativeIncludeFilePath << ")" << endl << endl;
                    } else if (suffix == "h") {
                        fileStream << "#include \"" << relativeIncludeFilePath << "\"" << endl << endl;
                    }

                    fileStream.flush();
                    currentFile.close();
                }
            } else {
                intermediate = QFile::copy(entry.absoluteFilePath(), destFile);
                SetFileAttributes((wchar_t*)destFile.utf16(), FILE_ATTRIBUTE_NORMAL);
            }
        }
	if(!intermediate) {
	    qDebug() << "cpdir: Failure for " << entry.fileName() << entry.isDir();
	    result = false;
	}
    }
    return result;
}

bool Environment::rmdir(const QString &name)
{
    bool result = true;
    QString cleanName = QDir::cleanPath(name);

    QDir dir = QDir(cleanName);
    QFileInfoList allEntries = dir.entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    for (int i = 0; result && (i < allEntries.count()); ++i) {
        QFileInfo entry = allEntries.at(i);
        if (entry.isDir()) {
            result &= rmdir(entry.absoluteFilePath());
        } else {
            result &= QFile::remove(entry.absoluteFilePath());
        }
    }
    result &= dir.rmdir(cleanName);
    return result;
}

QString Environment::symbianEpocRoot()
{
    // Call function defined in tools/shared/symbian/epocroot_p.h
    return ::qt_epocRoot();
}

QT_END_NAMESPACE
