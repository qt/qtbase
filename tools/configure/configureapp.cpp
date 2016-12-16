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

#include "configureapp.h"
#include "environment.h"

#include <qdir.h>
#include <qdiriterator.h>
#include <qtemporaryfile.h>
#include <qstandardpaths.h>
#include <qstack.h>
#include <qdebug.h>
#include <qfileinfo.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qhash.h>

#include <iostream>
#include <string>
#include <fstream>
#include <windows.h>

QT_BEGIN_NAMESPACE

std::ostream &operator<<(std::ostream &s, const QString &val) {
    s << val.toLocal8Bit().data();
    return s;
}


using namespace std;

Configure::Configure(int& argc, char** argv)
{
    int i;

    for (i = 1; i < argc; i++)
        configCmdLine += argv[ i ];

    if (configCmdLine.size() >= 2 && configCmdLine.at(0) == "-srcdir") {
        sourcePath = QDir::cleanPath(configCmdLine.at(1));
        sourceDir = QDir(sourcePath);
        configCmdLine.erase(configCmdLine.begin(), configCmdLine.begin() + 2);
    } else {
        // Get the path to the executable
        wchar_t module_name[MAX_PATH];
        GetModuleFileName(0, module_name, sizeof(module_name) / sizeof(wchar_t));
        QFileInfo sourcePathInfo = QString::fromWCharArray(module_name);
        sourcePath = sourcePathInfo.absolutePath();
        sourceDir = sourcePathInfo.dir();
    }
    buildPath = QDir::currentPath();
    if (sourceDir != buildDir) { //shadow builds!
        QDir(buildPath).mkpath("bin");

        buildDir.mkpath("mkspecs");
    }

    if (dictionary[ "QMAKESPEC" ].size() == 0) {
        dictionary[ "QMAKESPEC" ] = Environment::detectQMakeSpec();
        dictionary[ "QMAKESPEC_FROM" ] = "detected";
    }

    dictionary[ "SYNCQT" ]          = "auto";

    QString tmp = dictionary[ "QMAKESPEC" ];
    if (tmp.contains("\\")) {
        tmp = tmp.mid(tmp.lastIndexOf("\\") + 1);
    } else {
        tmp = tmp.mid(tmp.lastIndexOf("/") + 1);
    }
    dictionary[ "QMAKESPEC" ] = tmp;
}

Configure::~Configure()
{
}

void Configure::parseCmdLine()
{
    sourcePathMangled = sourcePath;
    buildPathMangled = buildPath;
    if (configCmdLine.size() && configCmdLine.at(0) == "-top-level") {
        dictionary[ "TOPLEVEL" ] = "yes";
        configCmdLine.removeAt(0);
        sourcePathMangled = QFileInfo(sourcePath).path();
        buildPathMangled = QFileInfo(buildPath).path();
    }
    qmakeCmdLine = configCmdLine;

    int argCount = configCmdLine.size();
    int i = 0;

    // Look first for -redo
    for (int k = 0 ; k < argCount; ++k) {
        if (configCmdLine.at(k) == "-redo") {
            configCmdLine.removeAt(k);
            if (!reloadCmdLine(k)) {
                dictionary["DONE"] = "error";
                return;
            }
            argCount = configCmdLine.size();
            break;
        }
    }

    for (; i<configCmdLine.size(); ++i) {
        if (configCmdLine.at(i) == "-platform") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QMAKESPEC" ] = configCmdLine.at(i);
        dictionary[ "QMAKESPEC_FROM" ] = "commandline";
        }

        else if (configCmdLine.at(i) == "-no-syncqt")
            dictionary[ "SYNCQT" ] = "no";

        else if (configCmdLine.at(i) == "-make-tool") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "MAKE" ] = configCmdLine.at(i);
        }

    }

    // Ensure that QMAKESPEC exists in the mkspecs folder
    const QString mkspecPath(sourcePath + "/mkspecs");
    QDirIterator itMkspecs(mkspecPath, QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    QStringList mkspecs;

    while (itMkspecs.hasNext()) {
        QString mkspec = itMkspecs.next();
        // Remove base PATH
        mkspec.remove(0, mkspecPath.length() + 1);
        mkspecs << mkspec;
    }

    if (dictionary["QMAKESPEC"].toLower() == "features"
        || !mkspecs.contains(dictionary["QMAKESPEC"], Qt::CaseInsensitive)) {
        dictionary[ "DONE" ] = "error";
        if (dictionary ["QMAKESPEC_FROM"] == "commandline") {
            cout << "Invalid option \"" << dictionary["QMAKESPEC"] << "\" for -platform." << endl;
        } else { // was autodetected from environment
            cout << "Unable to detect the platform from environment. Use -platform command line" << endl
                 << "argument and run configure again." << endl;
        }
        cout << "See the README file for a list of supported operating systems and compilers." << endl;
    } else {
        if (dictionary[ "QMAKESPEC" ].endsWith("-icc") ||
            dictionary[ "QMAKESPEC" ].endsWith("-msvc2012") ||
            dictionary[ "QMAKESPEC" ].endsWith("-msvc2013") ||
            dictionary[ "QMAKESPEC" ].endsWith("-msvc2015") ||
            dictionary[ "QMAKESPEC" ].endsWith("-msvc2017")) {
            if (dictionary[ "MAKE" ].isEmpty()) dictionary[ "MAKE" ] = "nmake";
            dictionary[ "QMAKEMAKEFILE" ] = "Makefile.win32";
        } else if (dictionary[ "QMAKESPEC" ].startsWith(QLatin1String("win32-g++"))) {
            if (dictionary[ "MAKE" ].isEmpty()) dictionary[ "MAKE" ] = "mingw32-make";
            dictionary[ "QMAKEMAKEFILE" ] = "Makefile.unix";
        } else {
            if (dictionary[ "MAKE" ].isEmpty()) dictionary[ "MAKE" ] = "make";
            dictionary[ "QMAKEMAKEFILE" ] = "Makefile.win32";
        }
    }
}

void Configure::generateHeaders()
{
    if (dictionary["SYNCQT"] == "auto")
        dictionary["SYNCQT"] = QFile::exists(sourcePath + "/.git") ? "yes" : "no";

    if (dictionary["SYNCQT"] == "yes") {
        if (!QStandardPaths::findExecutable(QStringLiteral("perl.exe")).isEmpty()) {
            cout << "Running syncqt..." << endl;
            QStringList args;
            args << "perl" << "-w";
            args += sourcePath + "/bin/syncqt.pl";
            args << "-version" << QT_VERSION_STR << "-minimal" << "-module" << "QtCore";
            args += sourcePath;
            int retc = Environment::execute(args, QStringList(), QStringList());
            if (retc) {
                cout << "syncqt failed, return code " << retc << endl << endl;
                dictionary["DONE"] = "error";
            }
        } else {
            cout << "Perl not found in environment - cannot run syncqt." << endl;
            dictionary["DONE"] = "error";
        }
    }
}

void Configure::buildQmake()
{
    {
        QStringList args;

        // Build qmake
        QString pwd = QDir::currentPath();
        if (!QDir(buildPath).mkpath("qmake")) {
            cout << "Cannot create qmake build dir." << endl;
            dictionary[ "DONE" ] = "error";
            return;
        }
        if (!QDir::setCurrent(buildPath + "/qmake")) {
            cout << "Cannot enter qmake build dir." << endl;
            dictionary[ "DONE" ] = "error";
            return;
        }

        QString makefile = "Makefile";
        {
            QFile out(makefile);
            if (out.open(QFile::WriteOnly | QFile::Text)) {
                QTextStream stream(&out);
                stream << "#AutoGenerated by configure.exe" << endl
                    << "BUILD_PATH = .." << endl
                    << "SOURCE_PATH = " << QDir::toNativeSeparators(sourcePath) << endl
                    << "INC_PATH = " << QDir::toNativeSeparators(
                           (QFile::exists(sourcePath + "/.git") ? ".." : sourcePath)
                           + "/include") << endl;
                stream << "QT_VERSION = " QT_VERSION_STR << endl
                       << "QT_MAJOR_VERSION = " QT_STRINGIFY(QT_VERSION_MAJOR) << endl
                       << "QT_MINOR_VERSION = " QT_STRINGIFY(QT_VERSION_MINOR) << endl
                       << "QT_PATCH_VERSION = " QT_STRINGIFY(QT_VERSION_PATCH) << endl;
                if (dictionary[ "QMAKESPEC" ].startsWith("win32-g++")) {
                    stream << "QMAKESPEC = $(SOURCE_PATH)\\mkspecs\\" << dictionary[ "QMAKESPEC" ] << endl
                           << "CONFIG_CXXFLAGS = -std=c++11 -ffunction-sections" << endl
                           << "CONFIG_LFLAGS = -Wl,--gc-sections" << endl;

                    QFile in(sourcePath + "/qmake/Makefile.unix.win32");
                    if (in.open(QFile::ReadOnly | QFile::Text))
                        stream << in.readAll();
                    QFile in2(sourcePath + "/qmake/Makefile.unix.mingw");
                    if (in2.open(QFile::ReadOnly | QFile::Text))
                        stream << in2.readAll();
                } else {
                    stream << "QMAKESPEC = " << dictionary["QMAKESPEC"] << endl;
                }

                stream << "\n\n";

                QFile in(sourcePath + "/qmake/" + dictionary["QMAKEMAKEFILE"]);
                if (in.open(QFile::ReadOnly | QFile::Text)) {
                    QString d = in.readAll();
                    //### need replaces (like configure.sh)? --Sam
                    stream << d << endl;
                }
                stream.flush();
                out.close();
            }
        }

        args += dictionary[ "MAKE" ];
        args += "-f";
        args += makefile;

        cout << "Creating qmake..." << endl;
        int exitCode = Environment::execute(args, QStringList(), QStringList());
        if (exitCode) {
            args.clear();
            args += dictionary[ "MAKE" ];
            args += "-f";
            args += makefile;
            args += "clean";
            exitCode = Environment::execute(args, QStringList(), QStringList());
            if (exitCode) {
                cout << "Cleaning qmake failed, return code " << exitCode << endl << endl;
                dictionary[ "DONE" ] = "error";
            } else {
                args.clear();
                args += dictionary[ "MAKE" ];
                args += "-f";
                args += makefile;
                exitCode = Environment::execute(args, QStringList(), QStringList());
                if (exitCode) {
                    cout << "Building qmake failed, return code " << exitCode << endl << endl;
                    dictionary[ "DONE" ] = "error";
                }
            }
        }
        QDir::setCurrent(pwd);
    }

    // Generate qt.conf
    QFile confFile(buildPath + "/bin/qt.conf");
    if (confFile.open(QFile::WriteOnly | QFile::Text)) { // Truncates any existing file.
        QTextStream confStream(&confFile);
        confStream << "[EffectivePaths]" << endl
                   << "Prefix=.." << endl
                   << "[Paths]" << endl
                   << "TargetSpec=dummy" << endl
                   << "HostSpec=" << dictionary["QMAKESPEC"] << endl;
        if (sourcePath != buildPath)
            confStream << "[EffectiveSourcePaths]" << endl
                       << "Prefix=" << sourcePath << endl;

        confStream.flush();
        confFile.close();
    }

}

void Configure::configure()
{
    QStringList args;
    args << buildPath + "/bin/qmake"
         << sourcePathMangled
         << "--" << qmakeCmdLine;

    QString pwd = QDir::currentPath();
    QDir::setCurrent(buildPathMangled);
    if (int exitCode = Environment::execute(args, QStringList(), QStringList())) {
        cout << "Qmake failed, return code " << exitCode  << endl << endl;
        dictionary[ "DONE" ] = "error";
    }
    QDir::setCurrent(pwd);
}

bool Configure::reloadCmdLine(int idx)
{
        QFile inFile(buildPathMangled + "/config.opt");
        if (!inFile.open(QFile::ReadOnly)) {
            inFile.setFileName(buildPath + "/config.opt");
            if (!inFile.open(QFile::ReadOnly)) {
                inFile.setFileName(buildPath + "/configure.cache");
                if (!inFile.open(QFile::ReadOnly)) {
                    cout << "No config.opt present - cannot redo configuration." << endl;
                    return false;
                }
            }
        }
        QTextStream inStream(&inFile);
        while (!inStream.atEnd())
            configCmdLine.insert(idx++, inStream.readLine().trimmed());
        return true;
}

bool Configure::isDone()
{
    return !dictionary["DONE"].isEmpty();
}

bool Configure::isOk()
{
    return (dictionary[ "DONE" ] != "error");
}

QT_END_NAMESPACE
