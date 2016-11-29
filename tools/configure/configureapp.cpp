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
#include "tools.h"

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
#include <conio.h>

QT_BEGIN_NAMESPACE

enum Platforms {
    WINDOWS,
    WINDOWS_RT,
    QNX,
    ANDROID,
    OTHER
};

std::ostream &operator<<(std::ostream &s, const QString &val) {
    s << val.toLocal8Bit().data();
    return s;
}


using namespace std;

static inline void promptKeyPress()
{
    cout << "(Press any key to continue...)";
    if (_getch() == 3) // _Any_ keypress w/no echo(eat <Enter> for stdout)
        exit(0);      // Exit cleanly for Ctrl+C
}

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
#if 0
    const QString installPath = QString("C:\\Qt\\%1").arg(QT_VERSION_STR);
#else
    const QString installPath = buildPath;
#endif
    if (sourceDir != buildDir) { //shadow builds!
        QDir(buildPath).mkpath("bin");

        buildDir.mkpath("mkspecs");
        buildDir.mkpath("config.tests");
    }

    dictionary[ "QT_INSTALL_PREFIX" ] = installPath;

    if (dictionary[ "QMAKESPEC" ].size() == 0) {
        dictionary[ "QMAKESPEC" ] = Environment::detectQMakeSpec();
        dictionary[ "QMAKESPEC_FROM" ] = "detected";
    }

    dictionary[ "SYNCQT" ]          = "auto";

    //Only used when cross compiling.
    dictionary[ "QT_INSTALL_SETTINGS" ] = "/etc/xdg";

    dictionary[ "REDO" ]            = "no";

    dictionary[ "BUILDTYPE" ]      = "none";

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

QString Configure::formatPath(const QString &path)
{
    QString ret = QDir::cleanPath(path);
    // This amount of quoting is deemed sufficient.
    if (ret.contains(QLatin1Char(' '))) {
        ret.prepend(QLatin1Char('"'));
        ret.append(QLatin1Char('"'));
    }
    return ret;
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

    int argCount = configCmdLine.size();
    int i = 0;

    // Look first for -redo
    for (int k = 0 ; k < argCount; ++k) {
        if (configCmdLine.at(k) == "-redo") {
            dictionary["REDO"] = "yes";
            configCmdLine.removeAt(k);
            if (!reloadCmdLine(k)) {
                dictionary["DONE"] = "error";
                return;
            }
            argCount = configCmdLine.size();
            break;
        }
    }

    // Then look for XQMAKESPEC
    bool isDeviceMkspec = false;
    for (int j = 0 ; j < argCount; ++j)
    {
        if ((configCmdLine.at(j) == "-xplatform") || (configCmdLine.at(j) == "-device")) {
            isDeviceMkspec = (configCmdLine.at(j) == "-device");
            ++j;
            if (j == argCount)
                break;
            dictionary["XQMAKESPEC"] = configCmdLine.at(j);
            applySpecSpecifics();
            break;
        }
    }

    for (; i<configCmdLine.size(); ++i) {
        if (configCmdLine.at(i) == "-opensource") {
            dictionary[ "BUILDTYPE" ] = "opensource";
        }
        else if (configCmdLine.at(i) == "-commercial") {
            dictionary[ "BUILDTYPE" ] = "commercial";
        }
        else if (configCmdLine.at(i) == "-platform") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QMAKESPEC" ] = configCmdLine.at(i);
        dictionary[ "QMAKESPEC_FROM" ] = "commandline";
        } else if (configCmdLine.at(i) == "-xplatform"
                || configCmdLine.at(i) == "-device") {
            ++i;
            // do nothing
        } else if (configCmdLine.at(i) == "-device-option") {
            ++i;
            const QString option = configCmdLine.at(i);
            QString &devOpt = dictionary["DEVICE_OPTION"];
            if (!devOpt.isEmpty())
                devOpt.append("\n").append(option);
            else
                devOpt = option;
        }

        else if (configCmdLine.at(i) == "-no-syncqt")
            dictionary[ "SYNCQT" ] = "no";

        else if (configCmdLine.at(i) == "-confirm-license") {
            dictionary["LICENSE_CONFIRMED"] = "yes";
        }

        // Directories ----------------------------------------------
        else if (configCmdLine.at(i) == "-prefix") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_PREFIX" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-bindir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_BINS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-libexecdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_LIBEXECS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-libdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_LIBS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-docdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_DOCS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-headerdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_HEADERS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-plugindir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_PLUGINS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-importdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_IMPORTS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-qmldir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_QML" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-archdatadir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_ARCHDATA" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-datadir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_DATA" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-translationdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_TRANSLATIONS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-examplesdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_EXAMPLES" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-testsdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_INSTALL_TESTS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-sysroot") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "CFG_SYSROOT" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-hostprefix") {
            ++i;
            if (i == argCount || configCmdLine.at(i).startsWith('-'))
                dictionary[ "QT_HOST_PREFIX" ] = buildPath;
            else
                dictionary[ "QT_HOST_PREFIX" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-hostbindir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_HOST_BINS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-hostlibdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_HOST_LIBS" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-hostdatadir") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_HOST_DATA" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-extprefix") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "QT_EXT_PREFIX" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-make-tool") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "MAKE" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-sysconfdir") {
            ++i;
            if (i == argCount)
                break;
            dictionary["QT_INSTALL_SETTINGS"] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-android-ndk") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "ANDROID_NDK_ROOT" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-android-sdk") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "ANDROID_SDK_ROOT" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-android-ndk-platform") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "ANDROID_PLATFORM" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-android-ndk-host") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "ANDROID_HOST" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-android-arch") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "ANDROID_TARGET_ARCH" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-android-toolchain-version") {
            ++i;
            if (i == argCount)
                break;
            dictionary[ "ANDROID_NDK_TOOLCHAIN_VERSION" ] = configCmdLine.at(i);
        }

        else if (configCmdLine.at(i) == "-no-android-style-assets") {
            dictionary[ "ANDROID_STYLE_ASSETS" ] = "no";
        } else if (configCmdLine.at(i) == "-android-style-assets") {
            dictionary[ "ANDROID_STYLE_ASSETS" ] = "yes";
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

    if (isDeviceMkspec) {
        const QStringList devices = mkspecs.filter("devices/", Qt::CaseInsensitive);
        const QStringList family = devices.filter(dictionary["XQMAKESPEC"], Qt::CaseInsensitive);

        if (family.isEmpty()) {
            dictionary[ "DONE" ] = "error";
            cout << "Error: No device matching '" << dictionary["XQMAKESPEC"] << "'." << endl;
        } else if (family.size() > 1) {
            dictionary[ "DONE" ] = "error";

            cout << "Error: Multiple matches for device '" << dictionary["XQMAKESPEC"] << "'. Candidates are:" << endl;

            foreach (const QString &device, family)
                cout << "\t* " << device << endl;
        } else {
            Q_ASSERT(family.size() == 1);
            dictionary["XQMAKESPEC"] = family.at(0);
        }

    } else {
        // Ensure that -spec (XQMAKESPEC) exists in the mkspecs folder as well
        if (dictionary.contains("XQMAKESPEC") &&
                !mkspecs.contains(dictionary["XQMAKESPEC"], Qt::CaseInsensitive)) {
            dictionary[ "DONE" ] = "error";
            cout << "Invalid option \"" << dictionary["XQMAKESPEC"] << "\" for -xplatform." << endl;
        }
    }
}

/*!
    Modifies the default configuration based on given -platform option.
    Eg. switches to different default styles for Windows CE.
*/
void Configure::applySpecSpecifics()
{
    if (platform() == ANDROID)
        dictionary["ANDROID_STYLE_ASSETS"]  = "yes";
}

void Configure::prepareConfigTests()
{
    // Generate an empty .qmake.cache file for config.tests
    QDir buildDir(buildPath);
    bool success = true;
    if (!buildDir.exists("config.tests"))
        success = buildDir.mkdir("config.tests");

    QString fileName(buildPath + "/config.tests/.qmake.cache");
    QFile cacheFile(fileName);
    success &= cacheFile.open(QIODevice::WriteOnly);
    cacheFile.close();

    if (!success) {
        cout << "Failed to create file " << qPrintable(QDir::toNativeSeparators(fileName)) << endl;
        dictionary[ "DONE" ] = "error";
    }
}

void Configure::generateQDevicePri()
{
    FileWriter deviceStream(buildPath + "/mkspecs/qdevice.pri");
    if (dictionary.contains("DEVICE_OPTION")) {
        const QString devoptionlist = dictionary["DEVICE_OPTION"];
        const QStringList optionlist = devoptionlist.split(QStringLiteral("\n"));
        foreach (const QString &entry, optionlist)
            deviceStream << entry << "\n";
    }
    if (dictionary.contains("ANDROID_SDK_ROOT") && dictionary.contains("ANDROID_NDK_ROOT")) {
        deviceStream << "android_install {" << endl;
        deviceStream << "    DEFAULT_ANDROID_SDK_ROOT = " << formatPath(dictionary["ANDROID_SDK_ROOT"]) << endl;
        deviceStream << "    DEFAULT_ANDROID_NDK_ROOT = " << formatPath(dictionary["ANDROID_NDK_ROOT"]) << endl;
        if (dictionary.contains("ANDROID_HOST"))
            deviceStream << "    DEFAULT_ANDROID_NDK_HOST = " << dictionary["ANDROID_HOST"] << endl;
        else if (QSysInfo::WordSize == 64)
            deviceStream << "    DEFAULT_ANDROID_NDK_HOST = windows-x86_64" << endl;
        else
            deviceStream << "    DEFAULT_ANDROID_NDK_HOST = windows" << endl;
        QString android_arch(dictionary.contains("ANDROID_TARGET_ARCH")
                  ? dictionary["ANDROID_TARGET_ARCH"]
                  : QString("armeabi-v7a"));
        QString android_tc_vers(dictionary.contains("ANDROID_NDK_TOOLCHAIN_VERSION")
                  ? dictionary["ANDROID_NDK_TOOLCHAIN_VERSION"]
                  : QString("4.9"));

        bool targetIs64Bit = android_arch == QString("arm64-v8a")
                             || android_arch == QString("x86_64")
                             || android_arch == QString("mips64");
        QString android_platform(dictionary.contains("ANDROID_PLATFORM")
                                 ? dictionary["ANDROID_PLATFORM"]
                                 : (targetIs64Bit ? QString("android-21") : QString("android-9")));

        deviceStream << "    DEFAULT_ANDROID_PLATFORM = " << android_platform << endl;
        deviceStream << "    DEFAULT_ANDROID_TARGET_ARCH = " << android_arch << endl;
        deviceStream << "    DEFAULT_ANDROID_NDK_TOOLCHAIN_VERSION = " << android_tc_vers << endl;
        deviceStream << "}" << endl;
    }
    if (!deviceStream.flush())
        dictionary[ "DONE" ] = "error";
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

void Configure::addConfStr(int group, const QString &val)
{
    confStrOffsets[group] += ' ' + QString::number(confStringOff) + ',';
    confStrings[group] += "    \"" + val + "\\0\"\n";
    confStringOff += val.length() + 1;
}

void Configure::generateQConfigCpp()
{
    QString hostSpec = dictionary["QMAKESPEC"];
    QString targSpec = dictionary.contains("XQMAKESPEC") ? dictionary["XQMAKESPEC"] : hostSpec;

    dictionary["CFG_SYSROOT"] = QDir::cleanPath(dictionary["CFG_SYSROOT"]);

    bool qipempty = false;
    if (dictionary["QT_INSTALL_PREFIX"].isEmpty())
        qipempty = true;
    else
        dictionary["QT_INSTALL_PREFIX"] = QDir::cleanPath(dictionary["QT_INSTALL_PREFIX"]);

    bool sysrootifyPrefix;
    if (dictionary["QT_EXT_PREFIX"].isEmpty()) {
        dictionary["QT_EXT_PREFIX"] = dictionary["QT_INSTALL_PREFIX"];
        sysrootifyPrefix = !dictionary["CFG_SYSROOT"].isEmpty();
    } else {
        dictionary["QT_EXT_PREFIX"] = QDir::cleanPath(dictionary["QT_EXT_PREFIX"]);
        sysrootifyPrefix = false;
    }

    bool haveHpx;
    if (dictionary["QT_HOST_PREFIX"].isEmpty()) {
        dictionary["QT_HOST_PREFIX"] = (sysrootifyPrefix ? dictionary["CFG_SYSROOT"] : QString())
                                       + dictionary["QT_INSTALL_PREFIX"];
        haveHpx = false;
    } else {
        dictionary["QT_HOST_PREFIX"] = QDir::cleanPath(dictionary["QT_HOST_PREFIX"]);
        haveHpx = true;
    }

    static const struct {
        const char *basevar, *baseoption, *var, *option;
    } varmod[] = {
        { "INSTALL_", "-prefix", "DOCS", "-docdir" },
        { "INSTALL_", "-prefix", "HEADERS", "-headerdir" },
        { "INSTALL_", "-prefix", "LIBS", "-libdir" },
        { "INSTALL_", "-prefix", "LIBEXECS", "-libexecdir" },
        { "INSTALL_", "-prefix", "BINS", "-bindir" },
        { "INSTALL_", "-prefix", "PLUGINS", "-plugindir" },
        { "INSTALL_", "-prefix", "IMPORTS", "-importdir" },
        { "INSTALL_", "-prefix", "QML", "-qmldir" },
        { "INSTALL_", "-prefix", "ARCHDATA", "-archdatadir" },
        { "INSTALL_", "-prefix", "DATA", "-datadir" },
        { "INSTALL_", "-prefix", "TRANSLATIONS", "-translationdir" },
        { "INSTALL_", "-prefix", "EXAMPLES", "-examplesdir" },
        { "INSTALL_", "-prefix", "TESTS", "-testsdir" },
        { "INSTALL_", "-prefix", "SETTINGS", "-sysconfdir" },
        { "HOST_", "-hostprefix", "BINS", "-hostbindir" },
        { "HOST_", "-hostprefix", "LIBS", "-hostlibdir" },
        { "HOST_", "-hostprefix", "DATA", "-hostdatadir" },
    };

    bool prefixReminder = false;
    for (uint i = 0; i < sizeof(varmod) / sizeof(varmod[0]); i++) {
        QString path = QDir::cleanPath(
                    dictionary[QLatin1String("QT_") + varmod[i].basevar + varmod[i].var]);
        if (path.isEmpty())
            continue;
        QString base = dictionary[QLatin1String("QT_") + varmod[i].basevar + "PREFIX"];
        if (!path.startsWith(base)) {
            if (i != 13) {
                dictionary["PREFIX_COMPLAINTS"] += QLatin1String("\n        NOTICE: ")
                        + varmod[i].option + " is not a subdirectory of " + varmod[i].baseoption + ".";
                if (i < 13 ? qipempty : !haveHpx)
                    prefixReminder = true;
            }
        } else {
            path.remove(0, base.size());
            if (path.startsWith('/'))
                path.remove(0, 1);
        }
        dictionary[QLatin1String("QT_REL_") + varmod[i].basevar + varmod[i].var]
                = path.isEmpty() ? "." : path;
    }
    if (prefixReminder) {
        dictionary["PREFIX_COMPLAINTS"]
                += "\n        Maybe you forgot to specify -prefix/-hostprefix?";
    }

    if (!qipempty) {
        // If QT_INSTALL_* have not been specified on the command line,
        // default them here, unless prefix is empty (WinCE).

        if (dictionary["QT_REL_INSTALL_HEADERS"].isEmpty())
            dictionary["QT_REL_INSTALL_HEADERS"] = "include";

        if (dictionary["QT_REL_INSTALL_LIBS"].isEmpty())
            dictionary["QT_REL_INSTALL_LIBS"] = "lib";

        if (dictionary["QT_REL_INSTALL_BINS"].isEmpty())
            dictionary["QT_REL_INSTALL_BINS"] = "bin";

        if (dictionary["QT_REL_INSTALL_ARCHDATA"].isEmpty())
            dictionary["QT_REL_INSTALL_ARCHDATA"] = ".";
        if (dictionary["QT_REL_INSTALL_ARCHDATA"] != ".")
            dictionary["QT_REL_INSTALL_ARCHDATA_PREFIX"] = dictionary["QT_REL_INSTALL_ARCHDATA"] + '/';

        if (dictionary["QT_REL_INSTALL_LIBEXECS"].isEmpty()) {
            if (targSpec.startsWith("win"))
                dictionary["QT_REL_INSTALL_LIBEXECS"] = dictionary["QT_REL_INSTALL_ARCHDATA_PREFIX"] + "bin";
            else
                dictionary["QT_REL_INSTALL_LIBEXECS"] = dictionary["QT_REL_INSTALL_ARCHDATA_PREFIX"] + "libexec";
        }

        if (dictionary["QT_REL_INSTALL_PLUGINS"].isEmpty())
            dictionary["QT_REL_INSTALL_PLUGINS"] = dictionary["QT_REL_INSTALL_ARCHDATA_PREFIX"] + "plugins";

        if (dictionary["QT_REL_INSTALL_IMPORTS"].isEmpty())
            dictionary["QT_REL_INSTALL_IMPORTS"] = dictionary["QT_REL_INSTALL_ARCHDATA_PREFIX"] + "imports";

        if (dictionary["QT_REL_INSTALL_QML"].isEmpty())
            dictionary["QT_REL_INSTALL_QML"] = dictionary["QT_REL_INSTALL_ARCHDATA_PREFIX"] + "qml";

        if (dictionary["QT_REL_INSTALL_DATA"].isEmpty())
            dictionary["QT_REL_INSTALL_DATA"] = ".";
        if (dictionary["QT_REL_INSTALL_DATA"] != ".")
            dictionary["QT_REL_INSTALL_DATA_PREFIX"] = dictionary["QT_REL_INSTALL_DATA"] + '/';

        if (dictionary["QT_REL_INSTALL_DOCS"].isEmpty())
            dictionary["QT_REL_INSTALL_DOCS"] = dictionary["QT_REL_INSTALL_DATA_PREFIX"] + "doc";

        if (dictionary["QT_REL_INSTALL_TRANSLATIONS"].isEmpty())
            dictionary["QT_REL_INSTALL_TRANSLATIONS"] = dictionary["QT_REL_INSTALL_DATA_PREFIX"] + "translations";

        if (dictionary["QT_REL_INSTALL_EXAMPLES"].isEmpty())
            dictionary["QT_REL_INSTALL_EXAMPLES"] = "examples";

        if (dictionary["QT_REL_INSTALL_TESTS"].isEmpty())
            dictionary["QT_REL_INSTALL_TESTS"] = "tests";
    }

    if (dictionary["QT_REL_HOST_BINS"].isEmpty())
        dictionary["QT_REL_HOST_BINS"] = haveHpx ? "bin" : dictionary["QT_REL_INSTALL_BINS"];

    if (dictionary["QT_REL_HOST_LIBS"].isEmpty())
        dictionary["QT_REL_HOST_LIBS"] = haveHpx ? "lib" : dictionary["QT_REL_INSTALL_LIBS"];

    if (dictionary["QT_REL_HOST_DATA"].isEmpty())
        dictionary["QT_REL_HOST_DATA"] = haveHpx ? "." : dictionary["QT_REL_INSTALL_ARCHDATA"];

    confStringOff = 0;
    addConfStr(0, dictionary["QT_REL_INSTALL_DOCS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_HEADERS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_LIBS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_LIBEXECS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_BINS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_PLUGINS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_IMPORTS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_QML"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_ARCHDATA"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_DATA"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_TRANSLATIONS"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_EXAMPLES"]);
    addConfStr(0, dictionary["QT_REL_INSTALL_TESTS"]);
    addConfStr(1, dictionary["CFG_SYSROOT"]);
    addConfStr(1, dictionary["QT_REL_HOST_BINS"]);
    addConfStr(1, dictionary["QT_REL_HOST_LIBS"]);
    addConfStr(1, dictionary["QT_REL_HOST_DATA"]);
    addConfStr(1, targSpec);
    addConfStr(1, hostSpec);

    // Generate the new qconfig.cpp file
    {
        FileWriter tmpStream(buildPath + "/src/corelib/global/qconfig.cpp");
        tmpStream << "/* Build date */" << endl
                  << "static const char qt_configure_installation          [11  + 12] = \"qt_instdate=2012-12-20\";" << endl
                  << endl
                  << "/* Installation Info */" << endl
                  << "static const char qt_configure_prefix_path_str       [512 + 12] = \"qt_prfxpath=" << dictionary["QT_INSTALL_PREFIX"] << "\";" << endl
                  << "#ifdef QT_BUILD_QMAKE" << endl
                  << "static const char qt_configure_ext_prefix_path_str   [512 + 12] = \"qt_epfxpath=" << dictionary["QT_EXT_PREFIX"] << "\";" << endl
                  << "static const char qt_configure_host_prefix_path_str  [512 + 12] = \"qt_hpfxpath=" << dictionary["QT_HOST_PREFIX"] << "\";" << endl
                  << "#endif" << endl
                  << endl
                  << "static const short qt_configure_str_offsets[] = {\n"
                  << "    " << confStrOffsets[0] << endl
                  << "#ifdef QT_BUILD_QMAKE\n"
                  << "    " << confStrOffsets[1] << endl
                  << "#endif\n"
                  << "};\n"
                  << "static const char qt_configure_strs[] =\n"
                  << confStrings[0] << "#ifdef QT_BUILD_QMAKE\n"
                  << confStrings[1] << "#endif\n"
                  << ";\n"
                  << endl;
        if ((platform() != WINDOWS) && (platform() != WINDOWS_RT))
            tmpStream << "#define QT_CONFIGURE_SETTINGS_PATH \"" << dictionary["QT_REL_INSTALL_SETTINGS"] << "\"" << endl;

        tmpStream << endl
                  << "#ifdef QT_BUILD_QMAKE\n"
                  << "# define QT_CONFIGURE_SYSROOTIFY_PREFIX " << (sysrootifyPrefix ? "true" : "false") << endl
                  << "#endif\n\n"
                  << endl
                  << "#define QT_CONFIGURE_PREFIX_PATH qt_configure_prefix_path_str + 12\n"
                  << "#ifdef QT_BUILD_QMAKE\n"
                  << "# define QT_CONFIGURE_EXT_PREFIX_PATH qt_configure_ext_prefix_path_str + 12\n"
                  << "# define QT_CONFIGURE_HOST_PREFIX_PATH qt_configure_host_prefix_path_str + 12\n"
                  << "#endif\n";

        if (!tmpStream.flush())
            dictionary[ "DONE" ] = "error";
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
                   << "Prefix=.." << endl;
        if (sourcePath != buildPath)
            confStream << "[EffectiveSourcePaths]" << endl
                       << "Prefix=" << sourcePath << endl;

        confStream.flush();
        confFile.close();
    }

}

void Configure::configure()
{
    FileWriter ci(buildPath + "/config.tests/configure.cfg");
    ci << "# Feature defaults set by configure command line\n"
       << "config.input.qt_edition = " << dictionary["EDITION"] << "\n"
       << "config.input.qt_licheck = " << dictionary["LICHECK"] << "\n"
       << "config.input.qt_release_date = " << dictionary["RELEASEDATE"];
    if (!ci.flush()) {
        dictionary[ "DONE" ] = "error";
        return;
    }

    QStringList args;
    args << buildPath + "/bin/qmake"
         << sourcePathMangled
         << "--" << configCmdLine;

    QString pwd = QDir::currentPath();
    QDir::setCurrent(buildPathMangled);
    if (int exitCode = Environment::execute(args, QStringList(), QStringList())) {
        cout << "Qmake failed, return code " << exitCode  << endl << endl;
        dictionary[ "DONE" ] = "error";
    }
    QDir::setCurrent(pwd);

    if ((dictionary["REDO"] != "yes") && (dictionary["DONE"] != "error"))
        saveCmdLine();
}

bool Configure::showLicense(QString orgLicenseFile)
{
    bool showGpl2 = true;
    QString licenseFile = orgLicenseFile;
    QString theLicense;
    if (dictionary["EDITION"] == "OpenSource") {
        if (platform() != WINDOWS_RT
                && (platform() != ANDROID || dictionary["ANDROID_STYLE_ASSETS"] == "no")) {
            theLicense = "GNU Lesser General Public License (LGPL) version 3\n"
                         "or the GNU General Public License (GPL) version 2";
        } else {
            theLicense = "GNU Lesser General Public License (LGPL) version 3";
            showGpl2 = false;
        }
    } else {
        // the first line of the license file tells us which license it is
        QFile file(licenseFile);
        if (!file.open(QFile::ReadOnly)) {
            cout << "Failed to load LICENSE file" << endl;
            return false;
        }
        theLicense = file.readLine().trimmed();
    }

    forever {
        char accept = '?';
        cout << "You are licensed to use this software under the terms of" << endl
             << "the " << theLicense << "." << endl
             << endl;
        if (dictionary["EDITION"] == "OpenSource") {
            cout << "Type 'L' to view the GNU Lesser General Public License version 3 (LGPLv3)." << endl;
            if (showGpl2)
                cout << "Type 'G' to view the GNU General Public License version 2 (GPLv2)." << endl;
        } else {
            cout << "Type '?' to view the " << theLicense << "." << endl;
        }
        cout << "Type 'y' to accept this license offer." << endl
             << "Type 'n' to decline this license offer." << endl
             << endl
             << "Do you accept the terms of the license?" << endl;
        cin >> accept;
        accept = tolower(accept);

        if (accept == 'y') {
            configCmdLine << "-confirm-license";
            return true;
        } else if (accept == 'n') {
            return false;
        } else {
            if (dictionary["EDITION"] == "OpenSource") {
                if (accept == 'l')
                    licenseFile = orgLicenseFile + "/LICENSE.LGPL3";
                else
                    licenseFile = orgLicenseFile + "/LICENSE.GPL2";
            }
            // Get console line height, to fill the screen properly
            int i = 0, screenHeight = 25; // default
            CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
            HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (GetConsoleScreenBufferInfo(stdOut, &consoleInfo))
                screenHeight = consoleInfo.srWindow.Bottom
                             - consoleInfo.srWindow.Top
                             - 1; // Some overlap for context

            // Prompt the license content to the user
            QFile file(licenseFile);
            if (!file.open(QFile::ReadOnly)) {
                cout << "Failed to load LICENSE file" << licenseFile << endl;
                return false;
            }
            QStringList licenseContent = QString(file.readAll()).split('\n');
            while (i < licenseContent.size()) {
                cout << licenseContent.at(i) << endl;
                if (++i % screenHeight == 0) {
                    promptKeyPress();
                    cout << "\r";     // Overwrite text above
                }
            }
        }
    }
}

void Configure::readLicense()
{
    dictionary["PLATFORM NAME"] = platformName();
    dictionary["LICENSE FILE"] = sourcePath;

    bool openSource = false;
    bool hasOpenSource = QFile::exists(dictionary["LICENSE FILE"] + "/LICENSE.LGPL3") || QFile::exists(dictionary["LICENSE FILE"] + "/LICENSE.GPL2");
    if (dictionary["BUILDTYPE"] == "commercial") {
        openSource = false;
    } else if (dictionary["BUILDTYPE"] == "opensource") {
        openSource = true;
    } else if (hasOpenSource) { // No Open Source? Just display the commercial license right away
        forever {
            char accept = '?';
            cout << "Which edition of Qt do you want to use ?" << endl;
            cout << "Type 'c' if you want to use the Commercial Edition." << endl;
            cout << "Type 'o' if you want to use the Open Source Edition." << endl;
            cin >> accept;
            accept = tolower(accept);

            if (accept == 'c') {
                openSource = false;
                break;
            } else if (accept == 'o') {
                openSource = true;
                break;
            }
        }
    }
    if (hasOpenSource && openSource) {
        cout << endl << "This is the " << dictionary["PLATFORM NAME"] << " Open Source Edition." << endl << endl;
        dictionary["LICENSEE"] = "Open Source";
        dictionary["EDITION"] = "OpenSource";
    } else if (openSource) {
        cout << endl << "Cannot find the GPL license files! Please download the Open Source version of the library." << endl;
        dictionary["DONE"] = "error";
        return;
    } else {
        QString tpLicense = sourcePath + "/LICENSE.PREVIEW.COMMERCIAL";
        if (QFile::exists(tpLicense)) {
            cout << endl << "This is the Qt Preview Edition." << endl << endl;

            dictionary["EDITION"] = "Preview";
            dictionary["LICENSE FILE"] = tpLicense;
        } else {
            Tools::checkLicense(dictionary, sourcePath, buildPath);
        }
    }

    if (dictionary["LICENSE_CONFIRMED"] != "yes") {
        if (!showLicense(dictionary["LICENSE FILE"])) {
            cout << "Configuration aborted since license was not accepted" << endl;
            dictionary["DONE"] = "error";
            return;
        }
    } else if (dictionary["LICHECK"].isEmpty()) { // licheck executable shows license
        cout << "You have already accepted the terms of the license." << endl << endl;
    }

    if (dictionary["BUILDTYPE"] == "none") {
        if (openSource)
            configCmdLine << "-opensource";
        else
            configCmdLine << "-commercial";
    }
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

void Configure::saveCmdLine()
{
    if (dictionary[ "REDO" ] != "yes") {
        QFile outFile(buildPathMangled + "/config.opt");
        if (outFile.open(QFile::WriteOnly | QFile::Text)) {
            QTextStream outStream(&outFile);
            for (QStringList::Iterator it = configCmdLine.begin(); it != configCmdLine.end(); ++it) {
                outStream << (*it) << endl;
            }
            outStream.flush();
            outFile.close();
        }
    }
}

bool Configure::isDone()
{
    return !dictionary["DONE"].isEmpty();
}

bool Configure::isOk()
{
    return (dictionary[ "DONE" ] != "error");
}

QString Configure::platformName() const
{
    switch (platform()) {
    default:
    case WINDOWS:
        return QStringLiteral("Qt for Windows");
    case WINDOWS_RT:
        return QStringLiteral("Qt for Windows Runtime");
    case QNX:
        return QStringLiteral("Qt for QNX");
    case ANDROID:
        return QStringLiteral("Qt for Android");
    case OTHER:
        return QStringLiteral("Qt for ???");
    }
}

int Configure::platform() const
{
    const QString xQMakeSpec = dictionary.value("XQMAKESPEC");

    if ((xQMakeSpec.startsWith("winphone") || xQMakeSpec.startsWith("winrt")))
        return WINDOWS_RT;

    if (xQMakeSpec.contains("qnx"))
        return QNX;

    if (xQMakeSpec.contains("android"))
        return ANDROID;

    if (!xQMakeSpec.isEmpty())
        return OTHER;

    return WINDOWS;
}

FileWriter::FileWriter(const QString &name)
    : QTextStream()
    , m_name(name)
{
    m_buffer.open(QIODevice::WriteOnly);
    setDevice(&m_buffer);
}

bool FileWriter::flush()
{
    QTextStream::flush();
    QFile oldFile(m_name);
    if (oldFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (oldFile.readAll() == m_buffer.data())
            return true;
        oldFile.close();
    }
    QString dir = QFileInfo(m_name).absolutePath();
    if (!QDir().mkpath(dir)) {
        cout << "Cannot create directory " << qPrintable(QDir::toNativeSeparators(dir)) << ".\n";
        return false;
    }
    QFile file(m_name + ".new");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (file.write(m_buffer.data()) == m_buffer.data().size()) {
            file.close();
            if (file.error() == QFile::NoError) {
                ::SetFileAttributes((wchar_t*)m_name.utf16(), FILE_ATTRIBUTE_NORMAL);
                QFile::remove(m_name);
                if (!file.rename(m_name)) {
                    cout << "Cannot replace file " << qPrintable(QDir::toNativeSeparators(m_name)) << ".\n";
                    return false;
                }
                return true;
            }
        }
    }
    cout << "Cannot create file " << qPrintable(QDir::toNativeSeparators(file.fileName()))
         << ": " << qPrintable(file.errorString()) << ".\n";
    file.remove();
    return false;
}

QT_END_NAMESPACE
