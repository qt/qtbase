/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QCoreApplication>
#include <QStringList>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>
#include <QDataStream>
#include <QXmlStreamReader>
#include <QDateTime>
#include <QStandardPaths>
#include <QUuid>
#include <QDirIterator>
#include <QRegExp>

#include <algorithm>

#if defined(Q_OS_WIN32)
#include <qt_windows.h>
#endif

#ifdef Q_CC_MSVC
#define popen _popen
#define QT_POPEN_READ "rb"
#define pclose _pclose
#else
#define QT_POPEN_READ "r"
#endif

class ActionTimer
{
    qint64 started;
public:
    ActionTimer() = default;
    void start()
    {
        started = QDateTime::currentMSecsSinceEpoch();
    }
    int elapsed()
    {
        return int(QDateTime::currentMSecsSinceEpoch() - started);
    }
};

static const bool mustReadOutputAnyway = true; // pclose seems to return the wrong error code unless we read the output

void deleteRecursively(const QString &dirName)
{
    QDir dir(dirName);
    if (!dir.exists())
        return;

    const QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir())
            deleteRecursively(entry.absoluteFilePath());
        else
            QFile::remove(entry.absoluteFilePath());
    }

    QDir().rmdir(dirName);
}

FILE *openProcess(const QString &command)
{
#if defined(Q_OS_WIN32)
    QString processedCommand = QLatin1Char('\"') + command + QLatin1Char('\"');
#else
    const QString& processedCommand = command;
#endif

    return popen(processedCommand.toLocal8Bit().constData(), QT_POPEN_READ);
}

struct QtDependency
{
    QtDependency(const QString &rpath, const QString &apath) : relativePath(rpath), absolutePath(apath) {}

    bool operator==(const QtDependency &other) const
    {
        return relativePath == other.relativePath && absolutePath == other.absolutePath;
    }

    QString relativePath;
    QString absolutePath;
};

struct Options
{
    Options()
        : helpRequested(false)
        , verbose(false)
        , timing(false)
        , build(true)
        , auxMode(false)
        , deploymentMechanism(Bundled)
        , releasePackage(false)
        , digestAlg(QLatin1String("SHA-256"))
        , sigAlg(QLatin1String("SHA256withRSA"))
        , internalSf(false)
        , sectionsOnly(false)
        , protectedAuthenticationPath(false)
        , jarSigner(false)
        , installApk(false)
        , uninstallApk(false)
    {}

    enum DeploymentMechanism
    {
        Bundled,
        Ministro
    };

    enum TriState {
        Auto,
        False,
        True
    };

    bool helpRequested;
    bool verbose;
    bool timing;
    bool build;
    bool auxMode;
    ActionTimer timer;

    // External tools
    QString sdkPath;
    QString sdkBuildToolsVersion;
    QString ndkPath;
    QString jdkPath;

    // Build paths
    QString qtInstallDirectory;
    std::vector<QString> extraPrefixDirs;
    QString androidSourceDirectory;
    QString outputDirectory;
    QString inputFileName;
    QString applicationBinary;
    QString rootPath;
    QStringList qmlImportPaths;
    QStringList qrcFiles;

    // Versioning
    QString versionName;
    QString versionCode;
    QByteArray minSdkVersion{"21"};
    QByteArray targetSdkVersion{"28"};

    // lib c++ path
    QString stdCppPath;
    QString stdCppName = QStringLiteral("c++_shared");

    // Build information
    QString androidPlatform;
    QHash<QString, QString> architectures;
    QString currentArchitecture;
    QString toolchainPrefix;
    QString ndkHost;
    bool buildAAB = false;


    // Package information
    DeploymentMechanism deploymentMechanism;
    QString packageName;
    QStringList extraLibs;
    QHash<QString, QStringList> archExtraLibs;
    QStringList extraPlugins;
    QHash<QString, QStringList> archExtraPlugins;

    // Signing information
    bool releasePackage;
    QString keyStore;
    QString keyStorePassword;
    QString keyStoreAlias;
    QString storeType;
    QString keyPass;
    QString sigFile;
    QString signedJar;
    QString digestAlg;
    QString sigAlg;
    QString tsaUrl;
    QString tsaCert;
    bool internalSf;
    bool sectionsOnly;
    bool protectedAuthenticationPath;
    bool jarSigner;
    QString apkPath;

    // Installation information
    bool installApk;
    bool uninstallApk;
    QString installLocation;

    // Per architecture collected information
    void clear(const QString &arch)
    {
        currentArchitecture = arch;
    }
    typedef QPair<QString, QString> BundledFile;
    QHash<QString, QList<BundledFile>> bundledFiles;
    QHash<QString, QList<QtDependency>> qtDependencies;
    QHash<QString, QStringList> localLibs;
    bool usesOpenGL = false;

    // Per package collected information
    QStringList localJars;
    QStringList initClasses;
    QStringList permissions;
    QStringList features;
};

static const QHash<QByteArray, QByteArray> elfArchitecures = {
    {"aarch64", "arm64-v8a"},
    {"arm", "armeabi-v7a"},
    {"i386", "x86"},
    {"x86_64", "x86_64"}
};

// Copy-pasted from qmake/library/ioutil.cpp
inline static bool hasSpecialChars(const QString &arg, const uchar (&iqm)[16])
{
    for (int x = arg.length() - 1; x >= 0; --x) {
        ushort c = arg.unicode()[x].unicode();
        if ((c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7))))
            return true;
    }
    return false;
}

static QString shellQuoteUnix(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    if (!arg.length())
        return QLatin1String("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        ret.replace(QLatin1Char('\''), QLatin1String("'\\''"));
        ret.prepend(QLatin1Char('\''));
        ret.append(QLatin1Char('\''));
    }
    return ret;
}

static QString shellQuoteWin(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    // - control chars & space
    // - the shell meta chars "&()<>^|
    // - the potential separators ,;=
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0x45, 0x13, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    };

    if (!arg.length())
        return QLatin1String("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        // Quotes are escaped and their preceding backslashes are doubled.
        // It's impossible to escape anything inside a quoted string on cmd
        // level, so the outer quoting must be "suspended".
        ret.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\"\\1\\1\\^\"\""));
        // The argument must not end with a \ since this would be interpreted
        // as escaping the quote -- rather put the \ behind the quote: e.g.
        // rather use "foo"\ than "foo\"
        int i = ret.length();
        while (i > 0 && ret.at(i - 1) == QLatin1Char('\\'))
            --i;
        ret.insert(i, QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    return ret;
}

static QString shellQuote(const QString &arg)
{
    if (QDir::separator() == QLatin1Char('\\'))
        return shellQuoteWin(arg);
    else
        return shellQuoteUnix(arg);
}

QString architecureFromName(const QString &name)
{
    QRegExp architecture(QStringLiteral(".*_(armeabi-v7a|arm64-v8a|x86|x86_64).so"));
    if (!architecture.exactMatch(name))
        return {};
    return architecture.capturedTexts().last();
}

QString fileArchitecture(const Options &options, const QString &path)
{
    auto arch = architecureFromName(path);
    if (!arch.isEmpty())
        return arch;

    QString readElf = QLatin1String("%1/toolchains/%2/prebuilt/%3/bin/llvm-readobj").arg(options.ndkPath,
                                                                                          options.toolchainPrefix,
                                                                                          options.ndkHost);
#if defined(Q_OS_WIN32)
    readElf += QLatin1String(".exe");
#endif

    if (!QFile::exists(readElf)) {
        fprintf(stderr, "Command does not exist: %s\n", qPrintable(readElf));
        return {};
    }

    readElf = QLatin1String("%1 -needed-libs %2").arg(shellQuote(readElf), shellQuote(path));

    FILE *readElfCommand = openProcess(readElf);
    if (!readElfCommand) {
        fprintf(stderr, "Cannot execute command %s\n", qPrintable(readElf));
        return {};
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), readElfCommand) != nullptr) {
        QByteArray line = QByteArray::fromRawData(buffer, qstrlen(buffer));
        QString library;
        line = line.trimmed();
        if (line.startsWith("Arch: ")) {
            auto it = elfArchitecures.find(line.mid(6));
            pclose(readElfCommand);
            return it != elfArchitecures.constEnd() ? QString::fromLatin1(it.value()) : QString{};
        }
    }
    pclose(readElfCommand);
    return {};
}

bool checkArchitecture(const Options &options, const QString &fileName)
{
    return fileArchitecture(options, fileName) == options.currentArchitecture;
}

void deleteMissingFiles(const Options &options, const QDir &srcDir, const QDir &dstDir)
{
    if (options.verbose)
        fprintf(stdout, "Delete missing files %s %s\n", qPrintable(srcDir.absolutePath()), qPrintable(dstDir.absolutePath()));

    const QFileInfoList srcEntries = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    const QFileInfoList dstEntries = dstDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    for (const QFileInfo &dst : dstEntries) {
        bool found = false;
        for (const QFileInfo &src : srcEntries)
            if (dst.fileName() == src.fileName()) {
                if (dst.isDir())
                    deleteMissingFiles(options, src.absoluteFilePath(), dst.absoluteFilePath());
                found = true;
                break;
            }

        if (!found) {
            if (options.verbose)
                fprintf(stdout, "%s not found in %s, removing it.\n", qPrintable(dst.fileName()), qPrintable(srcDir.absolutePath()));

            if (dst.isDir())
                deleteRecursively(dst.absolutePath());
            else
                QFile::remove(dst.absoluteFilePath());
        }
    }
    fflush(stdout);
}


Options parseOptions()
{
    Options options;

    QStringList arguments = QCoreApplication::arguments();
    for (int i=0; i<arguments.size(); ++i) {
        const QString &argument = arguments.at(i);
        if (argument.compare(QLatin1String("--output"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.outputDirectory = arguments.at(++i).trimmed();
        } else if (argument.compare(QLatin1String("--input"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.inputFileName = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--aab"), Qt::CaseInsensitive) == 0) {
            options.buildAAB = true;
            options.build = true;
            options.jarSigner = true;
        } else if (!options.buildAAB && argument.compare(QLatin1String("--no-build"), Qt::CaseInsensitive) == 0) {
            options.build = false;
        } else if (argument.compare(QLatin1String("--install"), Qt::CaseInsensitive) == 0) {
            options.installApk = true;
            options.uninstallApk = true;
        } else if (argument.compare(QLatin1String("--reinstall"), Qt::CaseInsensitive) == 0) {
            options.installApk = true;
            options.uninstallApk = false;
        } else if (argument.compare(QLatin1String("--android-platform"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.androidPlatform = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--help"), Qt::CaseInsensitive) == 0) {
            options.helpRequested = true;
        } else if (argument.compare(QLatin1String("--verbose"), Qt::CaseInsensitive) == 0) {
            options.verbose = true;
        } else if (argument.compare(QLatin1String("--deployment"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size()) {
                options.helpRequested = true;
            } else {
                QString deploymentMechanism = arguments.at(++i);
                if (deploymentMechanism.compare(QLatin1String("ministro"), Qt::CaseInsensitive) == 0) {
                    options.deploymentMechanism = Options::Ministro;
                } else if (deploymentMechanism.compare(QLatin1String("bundled"), Qt::CaseInsensitive) == 0) {
                    options.deploymentMechanism = Options::Bundled;
                } else {
                    fprintf(stderr, "Unrecognized deployment mechanism: %s\n", qPrintable(deploymentMechanism));
                    options.helpRequested = true;
                }
            }
        } else if (argument.compare(QLatin1String("--device"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.installLocation = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--release"), Qt::CaseInsensitive) == 0) {
            options.releasePackage = true;
        } else if (argument.compare(QLatin1String("--jdk"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.jdkPath = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--apk"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.apkPath = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--sign"), Qt::CaseInsensitive) == 0) {
            if (i + 2 >= arguments.size()) {
                options.helpRequested = true;
            } else {
                options.releasePackage = true;
                options.keyStore = arguments.at(++i);
                options.keyStoreAlias = arguments.at(++i);
            }
        } else if (argument.compare(QLatin1String("--storepass"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.keyStorePassword = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--storetype"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.storeType = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--keypass"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.keyPass = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--sigfile"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.sigFile = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--digestalg"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.digestAlg = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--sigalg"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.sigAlg = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--tsa"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.tsaUrl = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--tsacert"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.tsaCert = arguments.at(++i);
        } else if (argument.compare(QLatin1String("--internalsf"), Qt::CaseInsensitive) == 0) {
            options.internalSf = true;
        } else if (argument.compare(QLatin1String("--sectionsonly"), Qt::CaseInsensitive) == 0) {
            options.sectionsOnly = true;
        } else if (argument.compare(QLatin1String("--protected"), Qt::CaseInsensitive) == 0) {
            options.protectedAuthenticationPath = true;
        } else if (argument.compare(QLatin1String("--jarsigner"), Qt::CaseInsensitive) == 0) {
            options.jarSigner = true;
        } else if (argument.compare(QLatin1String("--aux-mode"), Qt::CaseInsensitive) == 0) {
            options.auxMode = true;
        }
    }

    if (options.inputFileName.isEmpty())
        options.inputFileName = QLatin1String("android-lib%1.so-deployment-settings.json").arg(QDir::current().dirName());

    options.timing = qEnvironmentVariableIsSet("ANDROIDDEPLOYQT_TIMING_OUTPUT");

    if (!QDir::current().mkpath(options.outputDirectory)) {
        fprintf(stderr, "Invalid output directory: %s\n", qPrintable(options.outputDirectory));
        options.outputDirectory.clear();
    } else {
        options.outputDirectory = QFileInfo(options.outputDirectory).canonicalFilePath();
        if (!options.outputDirectory.endsWith(QLatin1Char('/')))
            options.outputDirectory += QLatin1Char('/');
    }

    return options;
}

void printHelp()
{//                 "012345678901234567890123456789012345678901234567890123456789012345678901"
    fprintf(stderr, "Syntax: %s --output <destination> [options]\n"
                    "\n"
                    "  Creates an Android package in the build directory <destination> and\n"
                    "  builds it into an .apk file.\n\n"
                    "  Optional arguments:\n"
                    "    --input <inputfile>: Reads <inputfile> for options generated by\n"
                    "       qmake. A default file name based on the current working\n"
                    "       directory will be used if nothing else is specified.\n"
                    "    --deployment <mechanism>: Supported deployment mechanisms:\n"
                    "       bundled (default): Include Qt files in stand-alone package.\n"
                    "       ministro: Use the Ministro service to manage Qt files.\n"
                    "    --aab: Build an Android App Bundle.\n"
                    "    --no-build: Do not build the package, it is useful to just install\n"
                    "       a package previously built.\n"
                    "    --install: Installs apk to device/emulator. By default this step is\n"
                    "       not taken. If the application has previously been installed on\n"
                    "       the device, it will be uninstalled first.\n"
                    "    --reinstall: Installs apk to device/emulator. By default this step\n"
                    "       is not taken. If the application has previously been installed on\n"
                    "       the device, it will be overwritten, but its data will be left\n"
                    "       intact.\n"
                    "    --device [device ID]: Use specified device for deployment. Default\n"
                    "       is the device selected by default by adb.\n"
                    "    --android-platform <platform>: Builds against the given android\n"
                    "       platform. By default, the highest available version will be\n"
                    "       used.\n"
                    "    --release: Builds a package ready for release. By default, the\n"
                    "       package will be signed with a debug key.\n"
                    "    --sign <url/to/keystore> <alias>: Signs the package with the\n"
                    "       specified keystore, alias and store password. Also implies the\n"
                    "       --release option.\n"
                    "       Optional arguments for use with signing:\n"
                    "         --storepass <password>: Keystore password.\n"
                    "         --storetype <type>: Keystore type.\n"
                    "         --keypass <password>: Password for private key (if different\n"
                    "           from keystore password.)\n"
                    "         --sigfile <file>: Name of .SF/.DSA file.\n"
                    "         --digestalg <name>: Name of digest algorithm. Default is\n"
                    "           \"SHA1\".\n"
                    "         --sigalg <name>: Name of signature algorithm. Default is\n"
                    "           \"SHA1withRSA\".\n"
                    "         --tsa <url>: Location of the Time Stamping Authority.\n"
                    "         --tsacert <alias>: Public key certificate for TSA.\n"
                    "         --internalsf: Include the .SF file inside the signature block.\n"
                    "         --sectionsonly: Don't compute hash of entire manifest.\n"
                    "         --protected: Keystore has protected authentication path.\n"
                    "         --jarsigner: Force jarsigner usage, otherwise apksigner will be\n"
                    "           used if available.\n"
                    "    --jdk <path/to/jdk>: Used to find the jarsigner tool when used\n"
                    "       in combination with the --release argument. By default,\n"
                    "       an attempt is made to detect the tool using the JAVA_HOME and\n"
                    "       PATH environment variables, in that order.\n"
                    "    --qml-import-paths: Specify additional search paths for QML\n"
                    "       imports.\n"
                    "    --verbose: Prints out information during processing.\n"
                    "    --no-generated-assets-cache: Do not pregenerate the entry list for\n"
                    "       the assets file engine.\n"
                    "    --aux-mode: Operate in auxiliary mode. This will only copy the\n"
                    "       dependencies into the build directory and update the XML templates.\n"
                    "       The project will not be built or installed.\n"
                    "    --apk <path/where/to/copy/the/apk>: Path where to copy the built apk.\n"
                    "    --help: Displays this information.\n\n",
                    qPrintable(QCoreApplication::arguments().at(0))
            );
}

// Since strings compared will all start with the same letters,
// sorting by length and then alphabetically within each length
// gives the natural order.
bool quasiLexicographicalReverseLessThan(const QFileInfo &fi1, const QFileInfo &fi2)
{
    QString s1 = fi1.baseName();
    QString s2 = fi2.baseName();

    if (s1.length() == s2.length())
        return s1 > s2;
    else
        return s1.length() > s2.length();
}

// Files which contain templates that need to be overwritten by build data should be overwritten every
// time.
bool alwaysOverwritableFile(const QString &fileName)
{
    return (fileName.endsWith(QLatin1String("/res/values/libs.xml"))
            || fileName.endsWith(QLatin1String("/AndroidManifest.xml"))
            || fileName.endsWith(QLatin1String("/res/values/strings.xml"))
            || fileName.endsWith(QLatin1String("/src/org/qtproject/qt5/android/bindings/QtActivity.java")));
}


bool copyFileIfNewer(const QString &sourceFileName,
                     const QString &destinationFileName,
                     const Options &options,
                     bool forceOverwrite = false)
{
    if (QFile::exists(destinationFileName)) {
        QFileInfo destinationFileInfo(destinationFileName);
        QFileInfo sourceFileInfo(sourceFileName);

        if (!forceOverwrite
                && sourceFileInfo.lastModified() <= destinationFileInfo.lastModified()
                && !alwaysOverwritableFile(destinationFileName)) {
            if (options.verbose)
                fprintf(stdout, "  -- Skipping file %s. Same or newer file already in place.\n", qPrintable(sourceFileName));
            return true;
        } else {
            if (!QFile(destinationFileName).remove()) {
                fprintf(stderr, "Can't remove old file: %s\n", qPrintable(destinationFileName));
                return false;
            }
        }
    }

    if (!QDir().mkpath(QFileInfo(destinationFileName).path())) {
        fprintf(stderr, "Cannot make output directory for %s.\n", qPrintable(destinationFileName));
        return false;
    }

    if (!QFile::exists(destinationFileName) && !QFile::copy(sourceFileName, destinationFileName)) {
        fprintf(stderr, "Failed to copy %s to %s.\n", qPrintable(sourceFileName), qPrintable(destinationFileName));
        return false;
    } else if (options.verbose) {
        fprintf(stdout, "  -- Copied %s\n", qPrintable(destinationFileName));
        fflush(stdout);
    }
    return true;
}

QString cleanPackageName(QString packageName)
{
    QRegExp legalChars(QLatin1String("[a-zA-Z0-9_\\.]"));

    for (int i = 0; i < packageName.length(); ++i) {
        if (!legalChars.exactMatch(packageName.mid(i, 1)))
            packageName[i] = QLatin1Char('_');
    }

    static QStringList keywords;
    if (keywords.isEmpty()) {
        keywords << QLatin1String("abstract") << QLatin1String("continue") << QLatin1String("for")
                 << QLatin1String("new") << QLatin1String("switch") << QLatin1String("assert")
                 << QLatin1String("default") << QLatin1String("if") << QLatin1String("package")
                 << QLatin1String("synchronized") << QLatin1String("boolean") << QLatin1String("do")
                 << QLatin1String("goto") << QLatin1String("private") << QLatin1String("this")
                 << QLatin1String("break") << QLatin1String("double") << QLatin1String("implements")
                 << QLatin1String("protected") << QLatin1String("throw") << QLatin1String("byte")
                 << QLatin1String("else") << QLatin1String("import") << QLatin1String("public")
                 << QLatin1String("throws") << QLatin1String("case") << QLatin1String("enum")
                 << QLatin1String("instanceof") << QLatin1String("return") << QLatin1String("transient")
                 << QLatin1String("catch") << QLatin1String("extends") << QLatin1String("int")
                 << QLatin1String("short") << QLatin1String("try") << QLatin1String("char")
                 << QLatin1String("final") << QLatin1String("interface") << QLatin1String("static")
                 << QLatin1String("void") << QLatin1String("class") << QLatin1String("finally")
                 << QLatin1String("long") << QLatin1String("strictfp") << QLatin1String("volatile")
                 << QLatin1String("const") << QLatin1String("float") << QLatin1String("native")
                 << QLatin1String("super") << QLatin1String("while");
    }

    // No keywords
    int index = -1;
    while (index < packageName.length()) {
        int next = packageName.indexOf(QLatin1Char('.'), index + 1);
        if (next == -1)
            next = packageName.length();
        QString word = packageName.mid(index + 1, next - index - 1);
        if (!word.isEmpty()) {
            QChar c = word[0];
            if ((c >= QChar(QLatin1Char('0')) && c<= QChar(QLatin1Char('9')))
                   || c == QLatin1Char('_')) {
                packageName.insert(index + 1, QLatin1Char('a'));
                index = next + 1;
                continue;
            }
        }
        if (keywords.contains(word)) {
            packageName.insert(next, QLatin1String("_"));
            index = next + 1;
        } else {
            index = next;
        }
    }

    return packageName;
}

QString detectLatestAndroidPlatform(const QString &sdkPath)
{
    QDir dir(sdkPath + QLatin1String("/platforms"));
    if (!dir.exists()) {
        fprintf(stderr, "Directory %s does not exist\n", qPrintable(dir.absolutePath()));
        return QString();
    }

    QFileInfoList fileInfos = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (fileInfos.isEmpty()) {
        fprintf(stderr, "No platforms found in %s", qPrintable(dir.absolutePath()));
        return QString();
    }

    std::sort(fileInfos.begin(), fileInfos.end(), quasiLexicographicalReverseLessThan);

    QFileInfo latestPlatform = fileInfos.first();
    return latestPlatform.baseName();
}

QString packageNameFromAndroidManifest(const QString &androidManifestPath)
{
    QFile androidManifestXml(androidManifestPath);
    if (androidManifestXml.open(QIODevice::ReadOnly)) {
        QXmlStreamReader reader(&androidManifestXml);
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isStartElement() && reader.name() == QLatin1String("manifest"))
                return cleanPackageName(
                            reader.attributes().value(QLatin1String("package")).toString());
        }
    }
    return {};
}

bool readInputFile(Options *options)
{
    QFile file(options->inputFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Cannot read from input file: %s\n", qPrintable(options->inputFileName));
        return false;
    }

    QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll());
    if (jsonDocument.isNull()) {
        fprintf(stderr, "Invalid json file: %s\n", qPrintable(options->inputFileName));
        return false;
    }

    QJsonObject jsonObject = jsonDocument.object();

    {
        QJsonValue sdkPath = jsonObject.value(QLatin1String("sdk"));
        if (sdkPath.isUndefined()) {
            fprintf(stderr, "No SDK path in json file %s\n", qPrintable(options->inputFileName));
            return false;
        }

        options->sdkPath = QDir::fromNativeSeparators(sdkPath.toString());

        if (options->androidPlatform.isEmpty()) {
            options->androidPlatform = detectLatestAndroidPlatform(options->sdkPath);
            if (options->androidPlatform.isEmpty())
                return false;
        } else {
            if (!QDir(options->sdkPath + QLatin1String("/platforms/") + options->androidPlatform).exists()) {
                fprintf(stderr, "Warning: Android platform '%s' does not exist in SDK.\n",
                        qPrintable(options->androidPlatform));
            }
        }
    }

    {

        const QJsonValue value = jsonObject.value(QLatin1String("sdkBuildToolsRevision"));
        if (!value.isUndefined())
            options->sdkBuildToolsVersion = value.toString();
    }

    {
        const QJsonValue qtInstallDirectory = jsonObject.value(QLatin1String("qt"));
        if (qtInstallDirectory.isUndefined()) {
            fprintf(stderr, "No Qt directory in json file %s\n", qPrintable(options->inputFileName));
            return false;
        }
        options->qtInstallDirectory = qtInstallDirectory.toString();
    }

    {
        const auto extraPrefixDirs = jsonObject.value(QLatin1String("extraPrefixDirs")).toArray();
        options->extraPrefixDirs.reserve(extraPrefixDirs.size());
        for (const auto &prefix : extraPrefixDirs) {
            options->extraPrefixDirs.push_back(prefix.toString());
        }
    }

    {
        const QJsonValue androidSourcesDirectory = jsonObject.value(QLatin1String("android-package-source-directory"));
        if (!androidSourcesDirectory.isUndefined())
            options->androidSourceDirectory = androidSourcesDirectory.toString();
    }

    {
        const QJsonValue androidVersionName = jsonObject.value(QLatin1String("android-version-name"));
        if (!androidVersionName.isUndefined())
            options->versionName = androidVersionName.toString();
        else
            options->versionName = QStringLiteral("1.0");
    }

    {
        const QJsonValue androidVersionCode = jsonObject.value(QLatin1String("android-version-code"));
        if (!androidVersionCode.isUndefined())
            options->versionCode = androidVersionCode.toString();
        else
            options->versionCode = QStringLiteral("1");
    }

    {
        const QJsonValue ver = jsonObject.value(QLatin1String("android-min-sdk-version"));
        if (!ver.isUndefined())
            options->minSdkVersion = ver.toString().toUtf8();
    }

    {
        const QJsonValue ver = jsonObject.value(QLatin1String("android-target-sdk-version"));
        if (!ver.isUndefined())
            options->targetSdkVersion = ver.toString().toUtf8();
    }

    {
        const QJsonObject targetArchitectures = jsonObject.value(QLatin1String("architectures")).toObject();
        if (targetArchitectures.isEmpty()) {
            fprintf(stderr, "No target architecture defined in json file.\n");
            return false;
        }
        for (auto it = targetArchitectures.constBegin(); it != targetArchitectures.constEnd(); ++it) {
            if (it.value().isUndefined()) {
                fprintf(stderr, "Invalid architecure.\n");
                return false;
            }
            if (it.value().isNull())
                continue;
            options->architectures.insert(it.key(), it.value().toString());
        }
    }

    {
        const QJsonValue ndk = jsonObject.value(QLatin1String("ndk"));
        if (ndk.isUndefined()) {
            fprintf(stderr, "No NDK path defined in json file.\n");
            return false;
        }
        options->ndkPath = ndk.toString();
    }

    {
        const QJsonValue toolchainPrefix = jsonObject.value(QLatin1String("toolchain-prefix"));
        if (toolchainPrefix.isUndefined()) {
            fprintf(stderr, "No toolchain prefix defined in json file.\n");
            return false;
        }
        options->toolchainPrefix = toolchainPrefix.toString();
    }

    {
        const QJsonValue ndkHost = jsonObject.value(QLatin1String("ndk-host"));
        if (ndkHost.isUndefined()) {
            fprintf(stderr, "No NDK host defined in json file.\n");
            return false;
        }
        options->ndkHost = ndkHost.toString();
    }

    {
        const QJsonValue extraLibs = jsonObject.value(QLatin1String("android-extra-libs"));
        if (!extraLibs.isUndefined())
            options->extraLibs = extraLibs.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
    }

    {
        const QJsonValue extraPlugins = jsonObject.value(QLatin1String("android-extra-plugins"));
        if (!extraPlugins.isUndefined())
            options->extraPlugins = extraPlugins.toString().split(QLatin1Char(','));
    }

    {
        const QJsonValue stdcppPath = jsonObject.value(QLatin1String("stdcpp-path"));
        if (stdcppPath.isUndefined()) {
            fprintf(stderr, "No stdcpp-path defined in json file.\n");
            return false;
        }
        options->stdCppPath = stdcppPath.toString();
    }

    {
        const QJsonValue qmlRootPath = jsonObject.value(QLatin1String("qml-root-path"));
        if (!qmlRootPath.isUndefined())
            options->rootPath = qmlRootPath.toString();
    }

    {
        const QJsonValue qmlImportPaths = jsonObject.value(QLatin1String("qml-import-paths"));
        if (!qmlImportPaths.isUndefined())
            options->qmlImportPaths = qmlImportPaths.toString().split(QLatin1Char(','));
    }

    {
        const QJsonValue applicationBinary = jsonObject.value(QLatin1String("application-binary"));
        if (applicationBinary.isUndefined()) {
            fprintf(stderr, "No application binary defined in json file.\n");
            return false;
        }
        options->applicationBinary = applicationBinary.toString();
        if (options->build) {
            for (auto it = options->architectures.constBegin(); it != options->architectures.constEnd(); ++it) {
                auto appBinaryPath = QLatin1String("%1/libs/%2/lib%3_%2.so").arg(options->outputDirectory, it.key(), options->applicationBinary);
                if (!QFile::exists(appBinaryPath)) {
                    fprintf(stderr, "Cannot find application binary in build dir %s.\n", qPrintable(appBinaryPath));
                    return false;
                }
            }
        }
    }

    {
        const QJsonValue deploymentDependencies = jsonObject.value(QLatin1String("deployment-dependencies"));
        if (!deploymentDependencies.isUndefined()) {
            QString deploymentDependenciesString = deploymentDependencies.toString();
            const auto dependencies = deploymentDependenciesString.splitRef(QLatin1Char(','));
            for (const QStringRef &dependency : dependencies) {
                QString path = options->qtInstallDirectory + QLatin1Char('/') + dependency;
                if (QFileInfo(path).isDir()) {
                    QDirIterator iterator(path, QDirIterator::Subdirectories);
                    while (iterator.hasNext()) {
                        iterator.next();
                        if (iterator.fileInfo().isFile()) {
                            QString subPath = iterator.filePath();
                            auto arch = fileArchitecture(*options, subPath);
                            if (!arch.isEmpty()) {
                                options->qtDependencies[arch].append(QtDependency(subPath.mid(options->qtInstallDirectory.length() + 1),
                                                                                  subPath));
                            } else if (options->verbose) {
                                fprintf(stderr, "Skipping \"%s\", unknown architecture\n", qPrintable(subPath));
                                fflush(stderr);
                            }
                        }
                    }
                } else {
                    auto arch = fileArchitecture(*options, path);
                    if (!arch.isEmpty()) {
                        options->qtDependencies[arch].append(QtDependency(dependency.toString(), path));
                    } else if (options->verbose) {
                        fprintf(stderr, "Skipping \"%s\", unknown architecture\n", qPrintable(path));
                        fflush(stderr);
                    }
                }
            }
        }
    }
    {
        const QJsonValue qrcFiles = jsonObject.value(QLatin1String("qrcFiles"));
        options->qrcFiles = qrcFiles.toString().split(QLatin1Char(','), Qt::SkipEmptyParts);
    }
    options->packageName = packageNameFromAndroidManifest(options->androidSourceDirectory + QLatin1String("/AndroidManifest.xml"));
    if (options->packageName.isEmpty())
        options->packageName = cleanPackageName(QLatin1String("org.qtproject.example.%1").arg(options->applicationBinary));

    return true;
}

bool copyFiles(const QDir &sourceDirectory, const QDir &destinationDirectory, const Options &options, bool forceOverwrite = false)
{
    const QFileInfoList entries = sourceDirectory.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            QDir dir(entry.absoluteFilePath());
            if (!destinationDirectory.mkpath(dir.dirName())) {
                fprintf(stderr, "Cannot make directory %s in %s\n", qPrintable(dir.dirName()), qPrintable(destinationDirectory.path()));
                return false;
            }

            if (!copyFiles(dir, QDir(destinationDirectory.path() + QLatin1Char('/') + dir.dirName()), options, forceOverwrite))
                return false;
        } else {
            QString destination = destinationDirectory.absoluteFilePath(entry.fileName());
            if (!copyFileIfNewer(entry.absoluteFilePath(), destination, options, forceOverwrite))
                return false;
        }
    }

    return true;
}

void cleanTopFolders(const Options &options, const QDir &srcDir, const QString &dstDir)
{
    const auto dirs = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);
    for (const QFileInfo &dir : dirs) {
        if (dir.fileName() != QLatin1String("libs"))
            deleteMissingFiles(options, dir.absoluteFilePath(), dstDir + dir.fileName());
    }
}

void cleanAndroidFiles(const Options &options)
{
    if (!options.androidSourceDirectory.isEmpty())
        cleanTopFolders(options, options.androidSourceDirectory, options.outputDirectory);

    cleanTopFolders(options, options.qtInstallDirectory + QLatin1String("/src/android/templates"), options.outputDirectory);
}

bool copyAndroidTemplate(const Options &options, const QString &androidTemplate, const QString &outDirPrefix = QString())
{
    QDir sourceDirectory(options.qtInstallDirectory + androidTemplate);
    if (!sourceDirectory.exists()) {
        fprintf(stderr, "Cannot find template directory %s\n", qPrintable(sourceDirectory.absolutePath()));
        return false;
    }

    QString outDir = options.outputDirectory + outDirPrefix;

    if (!QDir::current().mkpath(outDir)) {
        fprintf(stderr, "Cannot create output directory %s\n", qPrintable(options.outputDirectory));
        return false;
    }

    return copyFiles(sourceDirectory, QDir(outDir), options);
}

bool copyGradleTemplate(const Options &options)
{
    QDir sourceDirectory(options.qtInstallDirectory + QLatin1String("/src/3rdparty/gradle"));
    if (!sourceDirectory.exists()) {
        fprintf(stderr, "Cannot find template directory %s\n", qPrintable(sourceDirectory.absolutePath()));
        return false;
    }

    QString outDir(options.outputDirectory);
    if (!QDir::current().mkpath(outDir)) {
        fprintf(stderr, "Cannot create output directory %s\n", qPrintable(options.outputDirectory));
        return false;
    }

    return copyFiles(sourceDirectory, QDir(outDir), options);
}

bool copyAndroidTemplate(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Copying Android package template.\n");

    if (!copyGradleTemplate(options))
        return false;

    if (!copyAndroidTemplate(options, QLatin1String("/src/android/templates")))
        return false;

    return true;
}

bool copyAndroidSources(const Options &options)
{
    if (options.androidSourceDirectory.isEmpty())
        return true;

    if (options.verbose)
        fprintf(stdout, "Copying Android sources from project.\n");

    QDir sourceDirectory(options.androidSourceDirectory);
    if (!sourceDirectory.exists()) {
        fprintf(stderr, "Cannot find android sources in %s", qPrintable(options.androidSourceDirectory));
        return false;
    }

    return copyFiles(sourceDirectory, QDir(options.outputDirectory), options, true);
}

bool copyAndroidExtraLibs(Options *options)
{
    if (options->extraLibs.isEmpty())
        return true;

    if (options->verbose)
        fprintf(stdout, "Copying %d external libraries to package.\n", options->extraLibs.size());

    for (const QString &extraLib : options->extraLibs) {
        QFileInfo extraLibInfo(extraLib);
        if (!extraLibInfo.exists()) {
            fprintf(stderr, "External library %s does not exist!\n", qPrintable(extraLib));
            return false;
        }
        if (!checkArchitecture(*options, extraLibInfo.filePath())) {
            if (options->verbose)
                fprintf(stdout, "Skipping \"%s\", architecture mismatch.\n", qPrintable(extraLib));
            continue;
        }
        if (!extraLibInfo.fileName().startsWith(QLatin1String("lib")) || extraLibInfo.suffix() != QLatin1String("so")) {
            fprintf(stderr, "The file name of external library %s must begin with \"lib\" and end with the suffix \".so\".\n",
                    qPrintable(extraLib));
            return false;
        }
        QString destinationFile(options->outputDirectory
                                + QLatin1String("/libs/")
                                + options->currentArchitecture
                                + QLatin1Char('/')
                                + extraLibInfo.fileName());

        if (!copyFileIfNewer(extraLib, destinationFile, *options))
            return false;
        options->archExtraLibs[options->currentArchitecture] += extraLib;
    }

    return true;
}

QStringList allFilesInside(const QDir& current, const QDir& rootDir)
{
    QStringList result;
    const auto dirs = current.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
    const auto files = current.entryList(QDir::Files);
    result.reserve(dirs.size() + files.size());
    for (const QString &dir : dirs) {
        result += allFilesInside(QDir(current.filePath(dir)), rootDir);
    }
    for (const QString &file : files) {
        result += rootDir.relativeFilePath(current.filePath(file));
    }
    return result;
}

bool copyAndroidExtraResources(Options *options)
{
    if (options->extraPlugins.isEmpty())
        return true;

    if (options->verbose)
        fprintf(stdout, "Copying %d external resources to package.\n", options->extraPlugins.size());

    for (const QString &extraResource : options->extraPlugins) {
        QFileInfo extraResourceInfo(extraResource);
        if (!extraResourceInfo.exists() || !extraResourceInfo.isDir()) {
            fprintf(stderr, "External resource %s does not exist or not a correct directory!\n", qPrintable(extraResource));
            return false;
        }

        QDir resourceDir(extraResource);
        QString assetsDir = options->outputDirectory + QLatin1String("/assets/") + resourceDir.dirName() + QLatin1Char('/');
        QString libsDir = options->outputDirectory + QLatin1String("/libs/") + options->currentArchitecture + QLatin1Char('/');

        const QStringList files = allFilesInside(resourceDir, resourceDir);
        for (const QString &resourceFile : files) {
            QString originFile(resourceDir.filePath(resourceFile));
            QString destinationFile;
            if (!resourceFile.endsWith(QLatin1String(".so"))) {
                destinationFile = assetsDir + resourceFile;
            } else {
                if (!checkArchitecture(*options, originFile))
                    continue;
                destinationFile = libsDir + resourceFile;
                options->archExtraPlugins[options->currentArchitecture] += resourceFile;
            }
            if (!copyFileIfNewer(originFile, destinationFile, *options))
                return false;
        }
    }

    return true;
}

bool updateFile(const QString &fileName, const QHash<QString, QString> &replacements)
{
    QFile inputFile(fileName);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Cannot open %s for reading.\n", qPrintable(fileName));
        return false;
    }

    // All the files we are doing substitutes in are quite small. If this
    // ever changes, this code should be updated to be more conservative.
    QByteArray contents = inputFile.readAll();

    bool hasReplacements = false;
    QHash<QString, QString>::const_iterator it;
    for (it = replacements.constBegin(); it != replacements.constEnd(); ++it) {
        if (it.key() == it.value())
            continue; // Nothing to actually replace

        forever {
            int index = contents.indexOf(it.key().toUtf8());
            if (index >= 0) {
                contents.replace(index, it.key().length(), it.value().toUtf8());
                hasReplacements = true;
            } else {
                break;
            }
        }
    }

    if (hasReplacements) {
        inputFile.close();

        if (!inputFile.open(QIODevice::WriteOnly)) {
            fprintf(stderr, "Cannot open %s for writing.\n", qPrintable(fileName));
            return false;
        }

        inputFile.write(contents);
    }

    return true;

}

bool updateLibsXml(Options *options)
{
    if (options->verbose)
        fprintf(stdout, "  -- res/values/libs.xml\n");

    QString fileName = options->outputDirectory + QLatin1String("/res/values/libs.xml");
    if (!QFile::exists(fileName)) {
        fprintf(stderr, "Cannot find %s in prepared packaged. This file is required.\n", qPrintable(fileName));
        return false;
    }

    QString qtLibs;
    QString allLocalLibs;
    QString extraLibs;

    for (auto it = options->architectures.constBegin(); it != options->architectures.constEnd(); ++it) {
        QString libsPath = QLatin1String("libs/") + it.key() + QLatin1Char('/');

        qtLibs += QLatin1String("        <item>%1;%2</item>\n").arg(it.key(), options->stdCppName);
        for (const Options::BundledFile &bundledFile : options->bundledFiles[it.key()]) {
            if (bundledFile.second.startsWith(QLatin1String("lib/"))) {
                QString s = bundledFile.second.mid(sizeof("lib/lib") - 1);
                s.chop(sizeof(".so") - 1);
                qtLibs += QLatin1String("        <item>%1;%2</item>\n").arg(it.key(), s);
            }
        }

        if (!options->archExtraLibs[it.key()].isEmpty()) {
            for (const QString &extraLib : options->archExtraLibs[it.key()]) {
                QFileInfo extraLibInfo(extraLib);
                QString name = extraLibInfo.fileName().mid(sizeof("lib") - 1);
                name.chop(sizeof(".so") - 1);
                extraLibs += QLatin1String("        <item>%1;%2</item>\n").arg(it.key(), name);
            }
        }

        QStringList localLibs;
        localLibs = options->localLibs[it.key()];
        // If .pro file overrides dependency detection, we need to see which platform plugin they picked
        if (localLibs.isEmpty()) {
            QString plugin;
            for (const QtDependency &qtDependency : options->qtDependencies[it.key()]) {
                if (qtDependency.relativePath.endsWith(QLatin1String("libqtforandroid.so"))
                        || qtDependency.relativePath.endsWith(QLatin1String("libqtforandroidGL.so"))) {
                    if (!plugin.isEmpty() && plugin != qtDependency.relativePath) {
                        fprintf(stderr, "Both platform plugins libqtforandroid.so and libqtforandroidGL.so included in package. Please include only one.\n");
                        return false;
                    }

                    plugin = qtDependency.relativePath;
                }
                if (qtDependency.relativePath.contains(QLatin1String("libQt5OpenGL"))
                        || qtDependency.relativePath.contains(QLatin1String("libQt5Quick"))) {
                    options->usesOpenGL |= true;
                    break;
                }
            }

            if (plugin.isEmpty()) {
                fflush(stdout);
                fprintf(stderr, "No platform plugin, neither libqtforandroid.so or libqtforandroidGL.so, included in package. Please include one.\n");
                fflush(stderr);
                return false;
            }

            localLibs.append(plugin);
            if (options->verbose)
                fprintf(stdout, "  -- Using platform plugin %s\n", qPrintable(plugin));
        }

        // remove all paths
        for (auto &lib : localLibs) {
            if (lib.endsWith(QLatin1String(".so")))
                lib = lib.mid(lib.lastIndexOf(QLatin1Char('/')) + 1);
        }
        allLocalLibs += QLatin1String("        <item>%1;%2</item>\n").arg(it.key(), localLibs.join(QLatin1Char(':')));
    }

    QHash<QString, QString> replacements;
    replacements[QStringLiteral("<!-- %%INSERT_QT_LIBS%% -->")] += qtLibs.trimmed();
    replacements[QStringLiteral("<!-- %%INSERT_LOCAL_LIBS%% -->")] = allLocalLibs.trimmed();
    replacements[QStringLiteral("<!-- %%INSERT_EXTRA_LIBS%% -->")] = extraLibs.trimmed();

    if (!updateFile(fileName, replacements))
        return false;

    return true;
}

bool updateStringsXml(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "  -- res/values/strings.xml\n");

    QHash<QString, QString> replacements;
    replacements[QStringLiteral("<!-- %%INSERT_APP_NAME%% -->")] = options.applicationBinary;

    QString fileName = options.outputDirectory + QLatin1String("/res/values/strings.xml");
    if (!QFile::exists(fileName)) {
        if (options.verbose)
            fprintf(stdout, "  -- Create strings.xml since it's missing.\n");
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            fprintf(stderr, "Can't open %s for writing.\n", qPrintable(fileName));
            return false;
        }
        file.write(QByteArray("<?xml version='1.0' encoding='utf-8'?><resources><string name=\"app_name\" translatable=\"false\">")
                   .append(options.applicationBinary.toLatin1())
                   .append("</string></resources>\n"));
        return true;
    }

    if (!updateFile(fileName, replacements))
        return false;

    return true;
}

bool updateAndroidManifest(Options &options)
{
    if (options.verbose)
        fprintf(stdout, "  -- AndroidManifest.xml \n");

    options.localJars.removeDuplicates();
    options.initClasses.removeDuplicates();

    QHash<QString, QString> replacements;
    replacements[QStringLiteral("-- %%INSERT_APP_NAME%% --")] = options.applicationBinary;
    replacements[QStringLiteral("-- %%INSERT_APP_LIB_NAME%% --")] = options.applicationBinary;
    replacements[QStringLiteral("-- %%INSERT_LOCAL_JARS%% --")] = options.localJars.join(QLatin1Char(':'));
    replacements[QStringLiteral("-- %%INSERT_INIT_CLASSES%% --")] = options.initClasses.join(QLatin1Char(':'));
    replacements[QStringLiteral("-- %%INSERT_VERSION_NAME%% --")] = options.versionName;
    replacements[QStringLiteral("-- %%INSERT_VERSION_CODE%% --")] = options.versionCode;
    replacements[QStringLiteral("package=\"org.qtproject.example\"")] = QLatin1String("package=\"%1\"").arg(options.packageName);
    replacements[QStringLiteral("-- %%BUNDLE_LOCAL_QT_LIBS%% --")]
            = (options.deploymentMechanism == Options::Bundled) ? QLatin1String("1") : QLatin1String("0");
    replacements[QStringLiteral("-- %%USE_LOCAL_QT_LIBS%% --")]
            = (options.deploymentMechanism != Options::Ministro) ? QLatin1String("1") : QLatin1String("0");

    QString permissions;
    for (const QString &permission : qAsConst(options.permissions))
        permissions += QLatin1String("    <uses-permission android:name=\"%1\" />\n").arg(permission);
    replacements[QStringLiteral("<!-- %%INSERT_PERMISSIONS -->")] = permissions.trimmed();

    QString features;
    for (const QString &feature : qAsConst(options.features))
        features += QLatin1String("    <uses-feature android:name=\"%1\" android:required=\"false\" />\n").arg(feature);
    if (options.usesOpenGL)
        features += QLatin1String("    <uses-feature android:glEsVersion=\"0x00020000\" android:required=\"true\" />");

    replacements[QStringLiteral("<!-- %%INSERT_FEATURES -->")] = features.trimmed();

    QString androidManifestPath = options.outputDirectory + QLatin1String("/AndroidManifest.xml");
    if (!updateFile(androidManifestPath, replacements))
        return false;

    // read the package, min & target sdk API levels from manifest file.
    bool checkOldAndroidLabelString = false;
    QFile androidManifestXml(androidManifestPath);
    if (androidManifestXml.exists()) {
        if (!androidManifestXml.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Cannot open %s for reading.\n", qPrintable(androidManifestPath));
            return false;
        }

        QXmlStreamReader reader(&androidManifestXml);
        while (!reader.atEnd()) {
            reader.readNext();

            if (reader.isStartElement()) {
                if (reader.name() == QLatin1String("manifest")) {
                    if (!reader.attributes().hasAttribute(QLatin1String("package"))) {
                        fprintf(stderr, "Invalid android manifest file: %s\n", qPrintable(androidManifestPath));
                        return false;
                    }
                    options.packageName = reader.attributes().value(QLatin1String("package")).toString();
                } else if (reader.name() == QLatin1String("uses-sdk")) {
                    if (reader.attributes().hasAttribute(QLatin1String("android:minSdkVersion")))
                        if (reader.attributes().value(QLatin1String("android:minSdkVersion")).toInt() < 21) {
                            fprintf(stderr, "Invalid minSdkVersion version, minSdkVersion must be >= 21\n");
                            return false;
                        }
                } else if ((reader.name() == QLatin1String("application") ||
                            reader.name() == QLatin1String("activity")) &&
                           reader.attributes().hasAttribute(QLatin1String("android:label")) &&
                           reader.attributes().value(QLatin1String("android:label")) == QLatin1String("@string/app_name")) {
                    checkOldAndroidLabelString = true;
                }
            }
        }

        if (reader.hasError()) {
            fprintf(stderr, "Error in %s: %s\n", qPrintable(androidManifestPath), qPrintable(reader.errorString()));
            return false;
        }
    } else {
        fprintf(stderr, "No android manifest file");
        return false;
    }

    if (checkOldAndroidLabelString)
        updateStringsXml(options);

    return true;
}

bool updateAndroidFiles(Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Updating Android package files with project settings.\n");

    if (!updateLibsXml(&options))
        return false;

    if (!updateAndroidManifest(options))
        return false;

    return true;
}

static QString absoluteFilePath(const Options *options, const QString &relativeFileName)
{
    for (const auto &prefix : options->extraPrefixDirs) {
        const QString path = prefix + QLatin1Char('/') + relativeFileName;
        if (QFile::exists(path))
            return path;
    }
    return options->qtInstallDirectory + QLatin1Char('/') + relativeFileName;
}

QList<QtDependency> findFilesRecursively(const Options &options, const QFileInfo &info, const QString &rootPath)
{
    if (!info.exists())
        return QList<QtDependency>();

    if (info.isDir()) {
        QList<QtDependency> ret;

        QDir dir(info.filePath());
        const QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString &entry : entries) {
            QString s = info.absoluteFilePath() + QLatin1Char('/') + entry;
            ret += findFilesRecursively(options, s, rootPath);
        }

        return ret;
    } else {
        return QList<QtDependency>() << QtDependency(info.absoluteFilePath().mid(rootPath.length()), info.absoluteFilePath());
    }
}

QList<QtDependency> findFilesRecursively(const Options &options, const QString &fileName)
{
    for (const auto &prefix : options.extraPrefixDirs) {
        QFileInfo info(prefix + QLatin1Char('/') + fileName);
        if (info.exists())
            return findFilesRecursively(options, info, prefix + QLatin1Char('/'));
    }
    QFileInfo info(options.qtInstallDirectory + QLatin1Char('/') + fileName);
    return findFilesRecursively(options, info, options.qtInstallDirectory + QLatin1Char('/'));
}

bool readAndroidDependencyXml(Options *options,
                              const QString &moduleName,
                              QSet<QString> *usedDependencies,
                              QSet<QString> *remainingDependencies)
{
    QString androidDependencyName = absoluteFilePath(options, QLatin1String("/lib/%1-android-dependencies.xml").arg(moduleName));

    QFile androidDependencyFile(androidDependencyName);
    if (androidDependencyFile.exists()) {
        if (options->verbose)
            fprintf(stdout, "Reading Android dependencies for %s\n", qPrintable(moduleName));

        if (!androidDependencyFile.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Cannot open %s for reading.\n", qPrintable(androidDependencyName));
            return false;
        }

        QXmlStreamReader reader(&androidDependencyFile);
        while (!reader.atEnd()) {
            reader.readNext();

            if (reader.isStartElement()) {
                if (reader.name() == QLatin1String("bundled")) {
                    if (!reader.attributes().hasAttribute(QLatin1String("file"))) {
                        fprintf(stderr, "Invalid android dependency file: %s\n", qPrintable(androidDependencyName));
                        return false;
                    }

                    QString file = reader.attributes().value(QLatin1String("file")).toString();

                    // Special case, since this is handled by qmlimportscanner instead
                    if (!options->rootPath.isEmpty() && (file == QLatin1String("qml") || file == QLatin1String("qml/")))
                        continue;

                    const QList<QtDependency> fileNames = findFilesRecursively(*options, file);
                    for (const QtDependency &fileName : fileNames) {
                        if (usedDependencies->contains(fileName.absolutePath))
                            continue;

                        usedDependencies->insert(fileName.absolutePath);

                        if (options->verbose)
                            fprintf(stdout, "Appending dependency from xml: %s\n", qPrintable(fileName.relativePath));

                        options->qtDependencies[options->currentArchitecture].append(fileName);
                    }
                } else if (reader.name() == QLatin1String("jar")) {
                    int bundling = reader.attributes().value(QLatin1String("bundling")).toInt();
                    QString fileName = reader.attributes().value(QLatin1String("file")).toString();
                    if (bundling == (options->deploymentMechanism == Options::Bundled)) {
                        QtDependency dependency(fileName, absoluteFilePath(options, fileName));
                        if (!usedDependencies->contains(dependency.absolutePath)) {
                            options->qtDependencies[options->currentArchitecture].append(dependency);
                            usedDependencies->insert(dependency.absolutePath);
                        }
                    }

                    if (!fileName.isEmpty())
                        options->localJars.append(fileName);

                    if (reader.attributes().hasAttribute(QLatin1String("initClass"))) {
                        options->initClasses.append(reader.attributes().value(QLatin1String("initClass")).toString());
                    }
                } else if (reader.name() == QLatin1String("lib")) {
                    QString fileName = reader.attributes().value(QLatin1String("file")).toString();
                    if (reader.attributes().hasAttribute(QLatin1String("replaces"))) {
                        QString replaces = reader.attributes().value(QLatin1String("replaces")).toString();
                        for (int i=0; i<options->localLibs.size(); ++i) {
                            if (options->localLibs[options->currentArchitecture].at(i) == replaces) {
                                options->localLibs[options->currentArchitecture][i] = fileName;
                                break;
                            }
                        }
                    } else if (!fileName.isEmpty()) {
                        options->localLibs[options->currentArchitecture].append(fileName);
                    }
                    if (fileName.endsWith(QLatin1String(".so")) && checkArchitecture(*options, fileName)) {
                        remainingDependencies->insert(fileName);
                    }
                } else if (reader.name() == QLatin1String("permission")) {
                    QString name = reader.attributes().value(QLatin1String("name")).toString();
                    options->permissions.append(name);
                } else if (reader.name() == QLatin1String("feature")) {
                    QString name = reader.attributes().value(QLatin1String("name")).toString();
                    options->features.append(name);
                }
            }
        }

        if (reader.hasError()) {
            fprintf(stderr, "Error in %s: %s\n", qPrintable(androidDependencyName), qPrintable(reader.errorString()));
            return false;
        }
    } else if (options->verbose) {
        fprintf(stdout, "No android dependencies for %s\n", qPrintable(moduleName));
    }
    options->permissions.removeDuplicates();
    options->features.removeDuplicates();

    return true;
}

QStringList getQtLibsFromElf(const Options &options, const QString &fileName)
{
    QString readElf = QLatin1String("%1/toolchains/%2/prebuilt/%3/bin/llvm-readobj").arg(options.ndkPath,
                                                                                          options.toolchainPrefix,
                                                                                          options.ndkHost);
#if defined(Q_OS_WIN32)
    readElf += QLatin1String(".exe");
#endif

    if (!QFile::exists(readElf)) {
        fprintf(stderr, "Command does not exist: %s\n", qPrintable(readElf));
        return QStringList();
    }

    readElf = QLatin1String("%1 -needed-libs %2").arg(shellQuote(readElf), shellQuote(fileName));

    FILE *readElfCommand = openProcess(readElf);
    if (!readElfCommand) {
        fprintf(stderr, "Cannot execute command %s\n", qPrintable(readElf));
        return QStringList();
    }

    QStringList ret;

    bool readLibs = false;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), readElfCommand) != nullptr) {
        QByteArray line = QByteArray::fromRawData(buffer, qstrlen(buffer));
        QString library;
        line = line.trimmed();
        if (!readLibs) {
            if (line.startsWith("Arch: ")) {
                auto it = elfArchitecures.find(line.mid(6));
                if (it == elfArchitecures.constEnd() || *it != options.currentArchitecture.toLatin1()) {
                    if (options.verbose)
                        fprintf(stdout, "Skipping \"%s\", architecture mismatch\n", qPrintable(fileName));
                    return {};
                }
            }
            readLibs = line.startsWith("NeededLibraries");
            continue;
        }
        if (!line.startsWith("lib"))
            continue;
        library = QString::fromLatin1(line);
        QString libraryName = QLatin1String("lib/") + library;
        if (QFile::exists(absoluteFilePath(&options, libraryName)))
            ret += libraryName;
    }

    pclose(readElfCommand);

    return ret;
}

bool readDependenciesFromElf(Options *options,
                             const QString &fileName,
                             QSet<QString> *usedDependencies,
                             QSet<QString> *remainingDependencies)
{
    // Get dependencies on libraries in $QTDIR/lib
    const QStringList dependencies = getQtLibsFromElf(*options, fileName);

    if (options->verbose) {
        fprintf(stdout, "Reading dependencies from %s\n", qPrintable(fileName));
        for (const QString &dep : dependencies)
            fprintf(stdout, "      %s\n", qPrintable(dep));
    }
    // Recursively add dependencies from ELF and supplementary XML information
    QList<QString> dependenciesToCheck;
    for (const QString &dependency : dependencies) {
        if (usedDependencies->contains(dependency))
            continue;

        QString absoluteDependencyPath = absoluteFilePath(options, dependency);
        usedDependencies->insert(dependency);
        if (!readDependenciesFromElf(options,
                              absoluteDependencyPath,
                              usedDependencies,
                              remainingDependencies)) {
            return false;
        }

        options->qtDependencies[options->currentArchitecture].append(QtDependency(dependency, absoluteDependencyPath));
        if (options->verbose)
            fprintf(stdout, "Appending dependency: %s\n", qPrintable(dependency));
        dependenciesToCheck.append(dependency);
    }

    for (const QString &dependency : qAsConst(dependenciesToCheck)) {
        QString qtBaseName = dependency.mid(sizeof("lib/lib") - 1);
        qtBaseName = qtBaseName.left(qtBaseName.size() - (sizeof(".so") - 1));
        if (!readAndroidDependencyXml(options, qtBaseName, usedDependencies, remainingDependencies)) {
            return false;
        }
    }

    return true;
}

bool goodToCopy(const Options *options, const QString &file, QStringList *unmetDependencies);

bool scanImports(Options *options, QSet<QString> *usedDependencies)
{
    if (options->verbose)
        fprintf(stdout, "Scanning for QML imports.\n");

    QString qmlImportScanner = options->qtInstallDirectory + QLatin1String("/bin/qmlimportscanner");
#if defined(Q_OS_WIN32)
    qmlImportScanner += QLatin1String(".exe");
#endif

    if (!QFile::exists(qmlImportScanner)) {
        fprintf(stderr, "qmlimportscanner not found: %s\n", qPrintable(qmlImportScanner));
        return true;
    }

    QString rootPath = options->rootPath;
    if (!options->qrcFiles.isEmpty()) {
        qmlImportScanner += QLatin1String(" -qrcFiles");
        for (const QString &qrcFile : options->qrcFiles)
            qmlImportScanner += QLatin1Char(' ') + shellQuote(qrcFile);
    }

    if (rootPath.isEmpty())
        rootPath = QFileInfo(options->inputFileName).absolutePath();
    else
        rootPath = QFileInfo(rootPath).absoluteFilePath();

    if (!rootPath.endsWith(QLatin1Char('/')))
        rootPath += QLatin1Char('/');

    qmlImportScanner += QLatin1String(" -rootPath %1").arg(shellQuote(rootPath));

    QStringList importPaths;
    importPaths += shellQuote(options->qtInstallDirectory + QLatin1String("/qml"));
    if (!rootPath.isEmpty())
        importPaths += shellQuote(rootPath);
    for (const QString &qmlImportPath : qAsConst(options->qmlImportPaths))
        importPaths += shellQuote(qmlImportPath);
    qmlImportScanner += QLatin1String(" -importPath %1").arg(importPaths.join(QLatin1Char(' ')));

    if (options->verbose) {
        fprintf(stdout, "Running qmlimportscanner with the following command: %s\n",
            qmlImportScanner.toLocal8Bit().constData());
    }

    FILE *qmlImportScannerCommand = popen(qmlImportScanner.toLocal8Bit().constData(), QT_POPEN_READ);
    if (qmlImportScannerCommand == 0) {
        fprintf(stderr, "Couldn't run qmlimportscanner.\n");
        return false;
    }

    QByteArray output;
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), qmlImportScannerCommand) != 0)
        output += QByteArray(buffer, qstrlen(buffer));

    QJsonDocument jsonDocument = QJsonDocument::fromJson(output);
    if (jsonDocument.isNull()) {
        fprintf(stderr, "Invalid json output from qmlimportscanner.\n");
        return false;
    }

    QJsonArray jsonArray = jsonDocument.array();
    for (int i=0; i<jsonArray.count(); ++i) {
        QJsonValue value = jsonArray.at(i);
        if (!value.isObject()) {
            fprintf(stderr, "Invalid format of qmlimportscanner output.\n");
            return false;
        }

        QJsonObject object = value.toObject();
        QString path = object.value(QLatin1String("path")).toString();
        if (path.isEmpty()) {
            fprintf(stderr, "Warning: QML import could not be resolved in any of the import paths: %s\n",
                    qPrintable(object.value(QLatin1String("name")).toString()));
        } else {
            if (options->verbose)
                fprintf(stdout, "  -- Adding '%s' as QML dependency\n", path.toLocal8Bit().constData());

            QFileInfo info(path);

            // The qmlimportscanner sometimes outputs paths that do not exist.
            if (!info.exists()) {
                if (options->verbose)
                    fprintf(stdout, "    -- Skipping because file does not exist.\n");
                continue;
            }

            QString absolutePath = info.absolutePath();
            if (!absolutePath.endsWith(QLatin1Char('/')))
                absolutePath += QLatin1Char('/');

            if (absolutePath.startsWith(rootPath)) {
                if (options->verbose)
                    fprintf(stdout, "    -- Skipping because file is in QML root path.\n");
                continue;
            }

            QString importPathOfThisImport;
            for (const QString &importPath : qAsConst(importPaths)) {
#if defined(Q_OS_WIN32)
                Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
                Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif
                QString cleanImportPath = QDir::cleanPath(importPath);
                if (info.absoluteFilePath().startsWith(cleanImportPath, caseSensitivity)) {
                    importPathOfThisImport = importPath;
                    break;
                }
            }

            if (importPathOfThisImport.isEmpty()) {
                fprintf(stderr, "Import found outside of import paths: %s.\n", qPrintable(info.absoluteFilePath()));
                return false;
            }

            QDir dir(importPathOfThisImport);
            importPathOfThisImport = dir.absolutePath() + QLatin1Char('/');

            const QList<QtDependency> fileNames = findFilesRecursively(*options, info, importPathOfThisImport);
            for (QtDependency fileName : fileNames) {
                if (usedDependencies->contains(fileName.absolutePath))
                    continue;

                usedDependencies->insert(fileName.absolutePath);

                if (options->verbose)
                    fprintf(stdout, "    -- Appending dependency found by qmlimportscanner: %s\n", qPrintable(fileName.absolutePath));

                // Put all imports in default import path in assets
                fileName.relativePath.prepend(QLatin1String("qml/"));
                options->qtDependencies[options->currentArchitecture].append(fileName);

                if (fileName.absolutePath.endsWith(QLatin1String(".so")) && checkArchitecture(*options, fileName.absolutePath)) {
                    QSet<QString> remainingDependencies;
                    if (!readDependenciesFromElf(options, fileName.absolutePath, usedDependencies, &remainingDependencies))
                        return false;

                }
            }
        }
    }

    return true;
}

bool runCommand(const Options &options, const QString &command)
{
    if (options.verbose)
        fprintf(stdout, "Running command '%s'\n", qPrintable(command));

    FILE *runCommand = openProcess(command);
    if (runCommand == nullptr) {
        fprintf(stderr, "Cannot run command '%s'\n", qPrintable(command));
        return false;
    }
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), runCommand) != nullptr) {
        if (options.verbose)
            fprintf(stdout, "%s", buffer);
    }
    pclose(runCommand);
    fflush(stdout);
    fflush(stderr);
    return true;
}

bool createRcc(const Options &options)
{
    auto assetsDir = QLatin1String("%1/assets").arg(options.outputDirectory);
    if (!QDir{QLatin1String("%1/android_rcc_bundle").arg(assetsDir)}.exists()) {
        fprintf(stdout, "Skipping createRCC\n");
        return true;
    }

    if (options.verbose)
        fprintf(stdout, "Create rcc bundle.\n");

    QString rcc = options.qtInstallDirectory + QLatin1String("/bin/rcc");
#if defined(Q_OS_WIN32)
    rcc += QLatin1String(".exe");
#endif

    if (!QFile::exists(rcc)) {
        fprintf(stderr, "rcc not found: %s\n", qPrintable(rcc));
        return false;
    }
    auto currentDir = QDir::currentPath();
    if (!QDir::setCurrent(QLatin1String("%1/android_rcc_bundle").arg(assetsDir))) {
        fprintf(stderr, "Cannot set current dir to: %s\n", qPrintable(QLatin1String("%1/android_rcc_bundle").arg(assetsDir)));
        return false;
    }

    bool res = runCommand(options, QLatin1String("%1 --project -o %2").arg(rcc, shellQuote(QLatin1String("%1/android_rcc_bundle.qrc").arg(assetsDir))));
    if (!res)
        return false;

    QFile::rename(QLatin1String("%1/android_rcc_bundle.qrc").arg(assetsDir), QLatin1String("%1/android_rcc_bundle/android_rcc_bundle.qrc").arg(assetsDir));

    res = runCommand(options, QLatin1String("%1 %2 --binary -o %3 android_rcc_bundle.qrc").arg(rcc, shellQuote(QLatin1String("--root=/android_rcc_bundle/")),
                                                                                               shellQuote(QLatin1String("%1/android_rcc_bundle.rcc").arg(assetsDir))));
    if (!QDir::setCurrent(currentDir)) {
        fprintf(stderr, "Cannot set current dir to: %s\n", qPrintable(currentDir));
        return false;
    }
    QFile::remove(QLatin1String("%1/android_rcc_bundle.qrc").arg(assetsDir));
    QDir{QLatin1String("%1/android_rcc_bundle").arg(assetsDir)}.removeRecursively();
    return res;
}

bool readDependencies(Options *options)
{
    if (options->verbose)
        fprintf(stdout, "Detecting dependencies of application.\n");

    // Override set in .pro file
    if (!options->qtDependencies[options->currentArchitecture].isEmpty()) {
        if (options->verbose)
            fprintf(stdout, "\tDependencies explicitly overridden in .pro file. No detection needed.\n");
        return true;
    }

    QSet<QString> usedDependencies;
    QSet<QString> remainingDependencies;

    // Add dependencies of application binary first
    if (!readDependenciesFromElf(options, QLatin1String("%1/libs/%2/lib%3_%2.so").arg(options->outputDirectory, options->currentArchitecture, options->applicationBinary), &usedDependencies, &remainingDependencies))
        return false;

    while (!remainingDependencies.isEmpty()) {
        QSet<QString>::iterator start = remainingDependencies.begin();
        QString fileName = absoluteFilePath(options, *start);
        remainingDependencies.erase(start);

        QStringList unmetDependencies;
        if (goodToCopy(options, fileName, &unmetDependencies)) {
            bool ok = readDependenciesFromElf(options, fileName, &usedDependencies, &remainingDependencies);
            if (!ok)
                return false;
        } else {
            fprintf(stdout, "Skipping %s due to unmet dependencies: %s\n",
                    qPrintable(fileName),
                    qPrintable(unmetDependencies.join(QLatin1Char(','))));
        }
    }

    QStringList::iterator it = options->localLibs[options->currentArchitecture].begin();
    while (it != options->localLibs[options->currentArchitecture].end()) {
        QStringList unmetDependencies;
        if (!goodToCopy(options, absoluteFilePath(options, *it), &unmetDependencies)) {
            fprintf(stdout, "Skipping %s due to unmet dependencies: %s\n",
                    qPrintable(*it),
                    qPrintable(unmetDependencies.join(QLatin1Char(','))));
            it = options->localLibs[options->currentArchitecture].erase(it);
        } else {
            ++it;
        }
    }

    if ((!options->rootPath.isEmpty() || options->qrcFiles.isEmpty()) &&
        !scanImports(options, &usedDependencies))
        return false;

    return true;
}

bool containsApplicationBinary(Options *options)
{
    if (!options->build)
        return true;

    if (options->verbose)
        fprintf(stdout, "Checking if application binary is in package.\n");

    QFileInfo applicationBinary(options->applicationBinary);
    QString applicationFileName = QLatin1String("lib%1_%2.so").arg(options->applicationBinary,
                                                                    options->currentArchitecture);

    QString applicationPath = QLatin1String("%1/libs/%2/%3").arg(options->outputDirectory,
                                                                               options->currentArchitecture,
                                                                               applicationFileName);
    if (!QFile::exists(applicationPath)) {
#if defined(Q_OS_WIN32)
        QLatin1String makeTool("mingw32-make"); // Only Mingw host builds supported on Windows currently
#else
        QLatin1String makeTool("make");
#endif
        fprintf(stderr, "Application binary is not in output directory: %s. Please run '%s install INSTALL_ROOT=%s' first.\n",
                qPrintable(applicationFileName),
                qPrintable(makeTool),
                qPrintable(options->outputDirectory));
        return false;
    }
    return true;
}

FILE *runAdb(const Options &options, const QString &arguments)
{
    QString adb = options.sdkPath + QLatin1String("/platform-tools/adb");
#if defined(Q_OS_WIN32)
    adb += QLatin1String(".exe");
#endif

    if (!QFile::exists(adb)) {
        fprintf(stderr, "Cannot find adb tool: %s\n", qPrintable(adb));
        return 0;
    }
    QString installOption;
    if (!options.installLocation.isEmpty())
        installOption = QLatin1String(" -s ") + shellQuote(options.installLocation);

    adb = QLatin1String("%1%2 %3").arg(shellQuote(adb), installOption, arguments);

    if (options.verbose)
        fprintf(stdout, "Running command \"%s\"\n", adb.toLocal8Bit().constData());

    FILE *adbCommand = openProcess(adb);
    if (adbCommand == 0) {
        fprintf(stderr, "Cannot start adb: %s\n", qPrintable(adb));
        return 0;
    }

    return adbCommand;
}

bool goodToCopy(const Options *options, const QString &file, QStringList *unmetDependencies)
{
    if (!file.endsWith(QLatin1String(".so")))
        return true;

    if (!checkArchitecture(*options, file))
        return false;

    bool ret = true;
    const auto libs = getQtLibsFromElf(*options, file);
    for (const QString &lib : libs) {
        if (!options->qtDependencies[options->currentArchitecture].contains(QtDependency(lib, absoluteFilePath(options, lib)))) {
            ret = false;
            unmetDependencies->append(lib);
        }
    }

    return ret;
}

bool copyQtFiles(Options *options)
{
    if (options->verbose) {
        switch (options->deploymentMechanism) {
        case Options::Bundled:
            fprintf(stdout, "Copying %d dependencies from Qt into package.\n", options->qtDependencies.size());
            break;
        case Options::Ministro:
            fprintf(stdout, "Setting %d dependencies from Qt in package.\n", options->qtDependencies.size());
            break;
        };
    }

    if (!options->build)
        return true;


    QString libsDirectory = QLatin1String("libs/");

    // Copy other Qt dependencies
    auto assetsDestinationDirectory = QLatin1String("assets/android_rcc_bundle/");
    for (const QtDependency &qtDependency : qAsConst(options->qtDependencies[options->currentArchitecture])) {
        QString sourceFileName = qtDependency.absolutePath;
        QString destinationFileName;

        if (qtDependency.relativePath.endsWith(QLatin1String(".so"))) {
            QString garbledFileName;
            if (qtDependency.relativePath.startsWith(QLatin1String("lib/"))) {
                garbledFileName = qtDependency.relativePath.mid(sizeof("lib/") - 1);
            } else {
                garbledFileName = qtDependency.relativePath.mid(qtDependency.relativePath.lastIndexOf(QLatin1Char('/')) + 1);
            }
            destinationFileName = libsDirectory + options->currentArchitecture + QLatin1Char('/') + garbledFileName;
        } else if (qtDependency.relativePath.startsWith(QLatin1String("jar/"))) {
            destinationFileName = libsDirectory + qtDependency.relativePath.mid(sizeof("jar/") - 1);
        } else {
            destinationFileName = assetsDestinationDirectory + qtDependency.relativePath;
        }

        if (!QFile::exists(sourceFileName)) {
            fprintf(stderr, "Source Qt file does not exist: %s.\n", qPrintable(sourceFileName));
            return false;
        }

        QStringList unmetDependencies;
        if (!goodToCopy(options, sourceFileName, &unmetDependencies)) {
            if (unmetDependencies.isEmpty()) {
                if (options->verbose) {
                    fprintf(stdout, "  -- Skipping %s, architecture mismatch.\n",
                            qPrintable(sourceFileName));
                }
            } else {
                if (unmetDependencies.isEmpty()) {
                    if (options->verbose) {
                        fprintf(stdout, "  -- Skipping %s, architecture mismatch.\n",
                                qPrintable(sourceFileName));
                    }
                } else {
                    fprintf(stdout, "  -- Skipping %s. It has unmet dependencies: %s.\n",
                            qPrintable(sourceFileName),
                            qPrintable(unmetDependencies.join(QLatin1Char(','))));
                }
            }
            continue;
        }

        if (options->deploymentMechanism == Options::Bundled
                && !copyFileIfNewer(sourceFileName,
                                    options->outputDirectory + QLatin1Char('/') + destinationFileName,
                                    *options)) {
            return false;
        }

        options->bundledFiles[options->currentArchitecture] += qMakePair(destinationFileName, qtDependency.relativePath);
    }

    return true;
}

QStringList getLibraryProjectsInOutputFolder(const Options &options)
{
    QStringList ret;

    QFile file(options.outputDirectory + QLatin1String("/project.properties"));
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QByteArray line = file.readLine().trimmed();
            if (line.startsWith("android.library.reference")) {
                int equalSignIndex = line.indexOf('=');
                if (equalSignIndex >= 0) {
                    QString path = QString::fromLocal8Bit(line.mid(equalSignIndex + 1));

                    QFileInfo info(options.outputDirectory + QLatin1Char('/') + path);
                    if (QDir::isRelativePath(path)
                            && info.exists()
                            && info.isDir()
                            && info.canonicalFilePath().startsWith(options.outputDirectory)) {
                        ret += info.canonicalFilePath();
                    }
                }
            }
        }
    }

    return ret;
}

bool createAndroidProject(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Running Android tool to create package definition.\n");

    QString androidToolExecutable = options.sdkPath + QLatin1String("/tools/android");
#if defined(Q_OS_WIN32)
    androidToolExecutable += QLatin1String(".bat");
#endif

    if (!QFile::exists(androidToolExecutable)) {
        fprintf(stderr, "Cannot find Android tool: %s\n", qPrintable(androidToolExecutable));
        return false;
    }

    QString androidTool = QLatin1String("%1 update project --path %2 --target %3 --name QtApp")
                            .arg(shellQuote(androidToolExecutable))
                            .arg(shellQuote(options.outputDirectory))
                            .arg(shellQuote(options.androidPlatform));

    if (options.verbose)
        fprintf(stdout, "  -- Command: %s\n", qPrintable(androidTool));

    FILE *androidToolCommand = openProcess(androidTool);
    if (androidToolCommand == 0) {
        fprintf(stderr, "Cannot run command '%s'\n", qPrintable(androidTool));
        return false;
    }

    pclose(androidToolCommand);

    // If the project has subprojects inside the current folder, we need to also run android update on these.
    const QStringList libraryProjects = getLibraryProjectsInOutputFolder(options);
    for (const QString &libraryProject : libraryProjects) {
        if (options.verbose)
            fprintf(stdout, "Updating subproject %s\n", qPrintable(libraryProject));

        androidTool = QLatin1String("%1 update lib-project --path %2 --target %3")
                .arg(shellQuote(androidToolExecutable))
                .arg(shellQuote(libraryProject))
                .arg(shellQuote(options.androidPlatform));

        if (options.verbose)
            fprintf(stdout, "  -- Command: %s\n", qPrintable(androidTool));

        FILE *androidToolCommand = popen(androidTool.toLocal8Bit().constData(), QT_POPEN_READ);
        if (androidToolCommand == 0) {
            fprintf(stderr, "Cannot run command '%s'\n", qPrintable(androidTool));
            return false;
        }

        pclose(androidToolCommand);
    }

    return true;
}

QString findInPath(const QString &fileName)
{
    const QString path = QString::fromLocal8Bit(qgetenv("PATH"));
#if defined(Q_OS_WIN32)
    QLatin1Char separator(';');
#else
    QLatin1Char separator(':');
#endif

    const QStringList paths = path.split(separator);
    for (const QString &path : paths) {
        QFileInfo fileInfo(path + QLatin1Char('/') + fileName);
        if (fileInfo.exists() && fileInfo.isFile() && fileInfo.isExecutable())
            return path + QLatin1Char('/') + fileName;
    }

    return QString();
}

typedef QMap<QByteArray, QByteArray> GradleProperties;

static GradleProperties readGradleProperties(const QString &path)
{
    GradleProperties properties;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return properties;

    const auto lines = file.readAll().split('\n');
    for (const QByteArray &line : lines) {
        if (line.trimmed().startsWith('#'))
            continue;

        QList<QByteArray> prop(line.split('='));
        if (prop.size() > 1)
            properties[prop.at(0).trimmed()] = prop.at(1).trimmed();
    }
    file.close();
    return properties;
}

static bool mergeGradleProperties(const QString &path, GradleProperties properties)
{
    QFile::remove(path + QLatin1Char('~'));
    QFile::rename(path, path + QLatin1Char('~'));
    QFile file(path);
    if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "Can't open file: %s for writing\n", qPrintable(file.fileName()));
        return false;
    }

    QFile oldFile(path + QLatin1Char('~'));
    if (oldFile.open(QIODevice::ReadOnly)) {
        while (!oldFile.atEnd()) {
            QByteArray line(oldFile.readLine());
            QList<QByteArray> prop(line.split('='));
            if (prop.size() > 1) {
                GradleProperties::iterator it = properties.find(prop.at(0).trimmed());
                if (it != properties.end()) {
                    file.write(it.key() + '=' + it.value() + '\n');
                    properties.erase(it);
                    continue;
                }
            }
            file.write(line);
        }
        oldFile.close();
    }

    for (GradleProperties::const_iterator it = properties.begin(); it != properties.end(); ++it)
        file.write(it.key() + '=' + it.value() + '\n');

    file.close();
    return true;
}

#if defined(Q_OS_WIN32)
void checkAndWarnGradleLongPaths(const QString &outputDirectory)
{
    QStringList longFileNames;
    QDirIterator it(outputDirectory, QStringList(QStringLiteral("*.java")), QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (it.next().size() >= MAX_PATH)
            longFileNames.append(it.next());
    }

    if (!longFileNames.isEmpty()) {
        fprintf(stderr,
                "The maximum path length that can be processed by Gradle on Windows is %d characters.\n"
                "Consider moving your project to reduce its path length.\n"
                "The following files have too long paths:\n%s.\n",
                MAX_PATH, qPrintable(longFileNames.join(QLatin1Char('\n'))));
    }
}
#endif

bool buildAndroidProject(const Options &options)
{
    GradleProperties localProperties;
    localProperties["sdk.dir"] = QDir::fromNativeSeparators(options.sdkPath).toUtf8();
    localProperties["ndk.dir"] = QDir::fromNativeSeparators(options.ndkPath).toUtf8();

    if (!mergeGradleProperties(options.outputDirectory + QLatin1String("local.properties"), localProperties))
        return false;

    QString gradlePropertiesPath = options.outputDirectory + QLatin1String("gradle.properties");
    GradleProperties gradleProperties = readGradleProperties(gradlePropertiesPath);
    gradleProperties["android.bundle.enableUncompressedNativeLibs"] = "false";
    gradleProperties["buildDir"] = "build";
    gradleProperties["qt5AndroidDir"] = (options.qtInstallDirectory + QLatin1String("/src/android/java")).toUtf8();
    gradleProperties["androidCompileSdkVersion"] = options.androidPlatform.split(QLatin1Char('-')).last().toLocal8Bit();
    gradleProperties["qtMinSdkVersion"] = options.minSdkVersion;
    gradleProperties["qtTargetSdkVersion"] = options.targetSdkVersion;
    if (gradleProperties["androidBuildToolsVersion"].isEmpty())
        gradleProperties["androidBuildToolsVersion"] = options.sdkBuildToolsVersion.toLocal8Bit();

    if (!mergeGradleProperties(gradlePropertiesPath, gradleProperties))
        return false;

#if defined(Q_OS_WIN32)
    QString gradlePath(options.outputDirectory + QLatin1String("gradlew.bat"));
#else
    QString gradlePath(options.outputDirectory + QLatin1String("gradlew"));
    {
        QFile f(gradlePath);
        if (!f.setPermissions(f.permissions() | QFileDevice::ExeUser))
            fprintf(stderr, "Cannot set permissions  %s\n", qPrintable(gradlePath));
    }
#endif

    QString oldPath = QDir::currentPath();
    if (!QDir::setCurrent(options.outputDirectory)) {
        fprintf(stderr, "Cannot current path to %s\n", qPrintable(options.outputDirectory));
        return false;
    }

    QString commandLine = QLatin1String("%1 %2").arg(shellQuote(gradlePath), options.releasePackage ? QLatin1String(" assembleRelease") : QLatin1String(" assembleDebug"));
    if (options.buildAAB)
        commandLine += QLatin1String(" bundle");

    if (options.verbose)
        commandLine += QLatin1String(" --info");

    FILE *gradleCommand = openProcess(commandLine);
    if (gradleCommand == 0) {
        fprintf(stderr, "Cannot run gradle command: %s\n.", qPrintable(commandLine));
        return false;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), gradleCommand) != 0) {
        fprintf(stdout, "%s", buffer);
        fflush(stdout);
    }

    int errorCode = pclose(gradleCommand);
    if (errorCode != 0) {
        fprintf(stderr, "Building the android package failed!\n");
        if (!options.verbose)
            fprintf(stderr, "  -- For more information, run this command with --verbose.\n");

#if defined(Q_OS_WIN32)
        checkAndWarnGradleLongPaths(options.outputDirectory);
#endif
        return false;
    }

    if (!QDir::setCurrent(oldPath)) {
        fprintf(stderr, "Cannot change back to old path: %s\n", qPrintable(oldPath));
        return false;
    }

    return true;
}

bool uninstallApk(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Uninstalling old Android package %s if present.\n", qPrintable(options.packageName));


    FILE *adbCommand = runAdb(options, QLatin1String(" uninstall ") + shellQuote(options.packageName));
    if (adbCommand == 0)
        return false;

    if (options.verbose || mustReadOutputAnyway) {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), adbCommand) != 0)
            if (options.verbose)
                fprintf(stdout, "%s", buffer);
    }

    int returnCode = pclose(adbCommand);
    if (returnCode != 0) {
        fprintf(stderr, "Warning: Uninstall failed!\n");
        if (!options.verbose)
            fprintf(stderr, "  -- Run with --verbose for more information.\n");
        return false;
    }

    return true;
}

enum PackageType {
    AAB,
    UnsignedAPK,
    SignedAPK
};

QString packagePath(const Options &options, PackageType pt)
{
    QString path(options.outputDirectory);
    path += QLatin1String("/build/outputs/%1/").arg(pt >= UnsignedAPK ? QStringLiteral("apk") : QStringLiteral("bundle"));
    QString buildType(options.releasePackage ? QLatin1String("release/") : QLatin1String("debug/"));
    if (QDir(path + buildType).exists())
        path += buildType;
    path += QDir(options.outputDirectory).dirName() + QLatin1Char('-');
    if (options.releasePackage) {
        path += QLatin1String("release-");
        if (pt >= UnsignedAPK) {
            if (pt == UnsignedAPK)
                path += QLatin1String("un");
            path += QLatin1String("signed.apk");
        } else {
            path.chop(1);
            path += QLatin1String(".aab");
        }
    } else {
        path += QLatin1String("debug");
        if (pt >= UnsignedAPK) {
            if (pt == SignedAPK)
                path += QLatin1String("-signed");
            path += QLatin1String(".apk");
        } else {
            path += QLatin1String(".aab");
        }
    }
    return shellQuote(path);
}

bool installApk(const Options &options)
{
    fflush(stdout);
    // Uninstall if necessary
    if (options.uninstallApk)
        uninstallApk(options);

    if (options.verbose)
        fprintf(stdout, "Installing Android package to device.\n");

    FILE *adbCommand = runAdb(options,
                              QLatin1String(" install -r ")
                              + packagePath(options, options.keyStore.isEmpty() ? UnsignedAPK
                                                                            : SignedAPK));
    if (adbCommand == 0)
        return false;

    if (options.verbose || mustReadOutputAnyway) {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), adbCommand) != 0)
            if (options.verbose)
                fprintf(stdout, "%s", buffer);
    }

    int returnCode = pclose(adbCommand);
    if (returnCode != 0) {
        fprintf(stderr, "Installing to device failed!\n");
        if (!options.verbose)
            fprintf(stderr, "  -- Run with --verbose for more information.\n");
        return false;
    }

    return true;
}

bool copyPackage(const Options &options)
{
    fflush(stdout);
    auto from = packagePath(options, options.keyStore.isEmpty() ? UnsignedAPK : SignedAPK);
    QFile::remove(options.apkPath);
    return QFile::copy(from, options.apkPath);
}

bool copyStdCpp(Options *options)
{
    if (options->verbose)
        fprintf(stdout, "Copying STL library\n");

    QString stdCppPath = QLatin1String("%1/%2/lib%3.so").arg(options->stdCppPath, options->architectures[options->currentArchitecture], options->stdCppName);
    if (!QFile::exists(stdCppPath)) {
        fprintf(stderr, "STL library does not exist at %s\n", qPrintable(stdCppPath));
        fflush(stdout);
        fflush(stderr);
        return false;
    }

    const QString destinationFile = QLatin1String("%1/libs/%2/lib%3.so").arg(options->outputDirectory,
                                                                              options->currentArchitecture,
                                                                              options->stdCppName);
    return copyFileIfNewer(stdCppPath, destinationFile, *options);
}

bool jarSignerSignPackage(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Signing Android package.\n");

    QString jdkPath = options.jdkPath;

    if (jdkPath.isEmpty())
        jdkPath = QString::fromLocal8Bit(qgetenv("JAVA_HOME"));

#if defined(Q_OS_WIN32)
    QString jarSignerTool = QLatin1String("jarsigner.exe");
#else
    QString jarSignerTool = QLatin1String("jarsigner");
#endif

    if (jdkPath.isEmpty() || !QFile::exists(jdkPath + QLatin1String("/bin/") + jarSignerTool))
        jarSignerTool = findInPath(jarSignerTool);
    else
        jarSignerTool = jdkPath + QLatin1String("/bin/") + jarSignerTool;

    if (!QFile::exists(jarSignerTool)) {
        fprintf(stderr, "Cannot find jarsigner in JAVA_HOME or PATH. Please use --jdk option to pass in the correct path to JDK.\n");
        return false;
    }

    jarSignerTool = QLatin1String("%1 -sigalg %2 -digestalg %3 -keystore %4")
            .arg(shellQuote(jarSignerTool), shellQuote(options.sigAlg), shellQuote(options.digestAlg), shellQuote(options.keyStore));

    if (!options.keyStorePassword.isEmpty())
        jarSignerTool += QLatin1String(" -storepass %1").arg(shellQuote(options.keyStorePassword));

    if (!options.storeType.isEmpty())
        jarSignerTool += QLatin1String(" -storetype %1").arg(shellQuote(options.storeType));

    if (!options.keyPass.isEmpty())
        jarSignerTool += QLatin1String(" -keypass %1").arg(shellQuote(options.keyPass));

    if (!options.sigFile.isEmpty())
        jarSignerTool += QLatin1String(" -sigfile %1").arg(shellQuote(options.sigFile));

    if (!options.signedJar.isEmpty())
        jarSignerTool += QLatin1String(" -signedjar %1").arg(shellQuote(options.signedJar));

    if (!options.tsaUrl.isEmpty())
        jarSignerTool += QLatin1String(" -tsa %1").arg(shellQuote(options.tsaUrl));

    if (!options.tsaCert.isEmpty())
        jarSignerTool += QLatin1String(" -tsacert %1").arg(shellQuote(options.tsaCert));

    if (options.internalSf)
        jarSignerTool += QLatin1String(" -internalsf");

    if (options.sectionsOnly)
        jarSignerTool += QLatin1String(" -sectionsonly");

    if (options.protectedAuthenticationPath)
        jarSignerTool += QLatin1String(" -protected");

    auto signPackage = [&](const QString &file) {
        fprintf(stdout, "Signing file %s\n", qPrintable(file));
        fflush(stdout);
        auto command = jarSignerTool + QLatin1String(" %1 %2")
                .arg(file)
                .arg(shellQuote(options.keyStoreAlias));

        FILE *jarSignerCommand = openProcess(command);
        if (jarSignerCommand == 0) {
            fprintf(stderr, "Couldn't run jarsigner.\n");
            return false;
        }

        if (options.verbose) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), jarSignerCommand) != 0)
                fprintf(stdout, "%s", buffer);
        }

        int errorCode = pclose(jarSignerCommand);
        if (errorCode != 0) {
            fprintf(stderr, "jarsigner command failed.\n");
            if (!options.verbose)
                fprintf(stderr, "  -- Run with --verbose for more information.\n");
            return false;
        }
        return true;
    };

    if (!signPackage(packagePath(options, UnsignedAPK)))
        return false;
    if (options.buildAAB && !signPackage(packagePath(options, AAB)))
        return false;

    QString zipAlignTool = options.sdkPath + QLatin1String("/tools/zipalign");
#if defined(Q_OS_WIN32)
    zipAlignTool += QLatin1String(".exe");
#endif

    if (!QFile::exists(zipAlignTool)) {
        zipAlignTool = options.sdkPath + QLatin1String("/build-tools/") + options.sdkBuildToolsVersion + QLatin1String("/zipalign");
#if defined(Q_OS_WIN32)
        zipAlignTool += QLatin1String(".exe");
#endif
        if (!QFile::exists(zipAlignTool)) {
            fprintf(stderr, "zipalign tool not found: %s\n", qPrintable(zipAlignTool));
            return false;
        }
    }

    zipAlignTool = QLatin1String("%1%2 -f 4 %3 %4")
            .arg(shellQuote(zipAlignTool),
                 options.verbose ? QLatin1String(" -v") : QLatin1String(),
                 packagePath(options, UnsignedAPK),
                 packagePath(options, SignedAPK));

    FILE *zipAlignCommand = openProcess(zipAlignTool);
    if (zipAlignCommand == 0) {
        fprintf(stderr, "Couldn't run zipalign.\n");
        return false;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), zipAlignCommand) != 0)
        fprintf(stdout, "%s", buffer);

    int errorCode = pclose(zipAlignCommand);
    if (errorCode != 0) {
        fprintf(stderr, "zipalign command failed.\n");
        if (!options.verbose)
            fprintf(stderr, "  -- Run with --verbose for more information.\n");
        return false;
    }

    return QFile::remove(packagePath(options, UnsignedAPK));
}

bool signPackage(const Options &options)
{
    QString apksignerTool = options.sdkPath + QLatin1String("/build-tools/") + options.sdkBuildToolsVersion + QLatin1String("/apksigner");
#if defined(Q_OS_WIN32)
    apksignerTool += QLatin1String(".bat");
#endif

    if (options.jarSigner || !QFile::exists(apksignerTool))
        return jarSignerSignPackage(options);

    // APKs signed with apksigner must not be changed after they're signed, therefore we need to zipalign it before we sign it.

    QString zipAlignTool = options.sdkPath + QLatin1String("/tools/zipalign");
#if defined(Q_OS_WIN32)
    zipAlignTool += QLatin1String(".exe");
#endif

    if (!QFile::exists(zipAlignTool)) {
        zipAlignTool = options.sdkPath + QLatin1String("/build-tools/") + options.sdkBuildToolsVersion + QLatin1String("/zipalign");
#if defined(Q_OS_WIN32)
        zipAlignTool += QLatin1String(".exe");
#endif
        if (!QFile::exists(zipAlignTool)) {
            fprintf(stderr, "zipalign tool not found: %s\n", qPrintable(zipAlignTool));
            return false;
        }
    }

    zipAlignTool = QLatin1String("%1%2 -f 4 %3 %4")
            .arg(shellQuote(zipAlignTool),
                 options.verbose ? QLatin1String(" -v") : QLatin1String(),
                 packagePath(options, UnsignedAPK),
                 packagePath(options, SignedAPK));

    FILE *zipAlignCommand = openProcess(zipAlignTool);
    if (zipAlignCommand == 0) {
        fprintf(stderr, "Couldn't run zipalign.\n");
        return false;
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), zipAlignCommand) != 0)
        fprintf(stdout, "%s", buffer);

    int errorCode = pclose(zipAlignCommand);
    if (errorCode != 0) {
        fprintf(stderr, "zipalign command failed.\n");
        if (!options.verbose)
            fprintf(stderr, "  -- Run with --verbose for more information.\n");
        return false;
    }

    QString apkSignerCommandLine = QLatin1String("%1 sign --ks %2")
            .arg(shellQuote(apksignerTool), shellQuote(options.keyStore));

    if (!options.keyStorePassword.isEmpty())
        apkSignerCommandLine += QLatin1String(" --ks-pass pass:%1").arg(shellQuote(options.keyStorePassword));

    if (!options.keyStoreAlias.isEmpty())
        apkSignerCommandLine += QLatin1String(" --ks-key-alias %1").arg(shellQuote(options.keyStoreAlias));

    if (!options.keyPass.isEmpty())
        apkSignerCommandLine += QLatin1String(" --key-pass pass:%1").arg(shellQuote(options.keyPass));

    if (options.verbose)
        apkSignerCommandLine += QLatin1String(" --verbose");

    apkSignerCommandLine += QLatin1String(" %1")
            .arg(packagePath(options, SignedAPK));

    auto apkSignerRunner = [&] {
        FILE *apkSignerCommand = openProcess(apkSignerCommandLine);
        if (apkSignerCommand == 0) {
            fprintf(stderr, "Couldn't run apksigner.\n");
            return false;
        }

        char buffer[512];
        while (fgets(buffer, sizeof(buffer), apkSignerCommand) != 0)
            fprintf(stdout, "%s", buffer);

        errorCode = pclose(apkSignerCommand);
        if (errorCode != 0) {
            fprintf(stderr, "apksigner command failed.\n");
            if (!options.verbose)
                fprintf(stderr, "  -- Run with --verbose for more information.\n");
            return false;
        }
        return true;
    };

    // Sign the package
    if (!apkSignerRunner())
        return false;

    apkSignerCommandLine = QLatin1String("%1 verify --verbose %2")
        .arg(shellQuote(apksignerTool), packagePath(options, SignedAPK));

    // Verify the package and remove the unsigned apk
    return apkSignerRunner() && QFile::remove(packagePath(options, UnsignedAPK));
}

enum ErrorCode
{
    Success,
    SyntaxErrorOrHelpRequested = 1,
    CannotReadInputFile = 2,
    CannotCopyAndroidTemplate = 3,
    CannotReadDependencies = 4,
    CannotCopyGnuStl = 5,
    CannotCopyQtFiles = 6,
    CannotFindApplicationBinary = 7,
    CannotCopyAndroidExtraLibs = 10,
    CannotCopyAndroidSources = 11,
    CannotUpdateAndroidFiles = 12,
    CannotCreateAndroidProject = 13,
    CannotBuildAndroidProject = 14,
    CannotSignPackage = 15,
    CannotInstallApk = 16,
    CannotCopyAndroidExtraResources = 19,
    CannotCopyApk = 20,
    CannotCreateRcc = 21
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Options options = parseOptions();
    if (options.helpRequested || options.outputDirectory.isEmpty()) {
        printHelp();
        return SyntaxErrorOrHelpRequested;
    }

    options.timer.start();

    if (!readInputFile(&options))
        return CannotReadInputFile;

    if (Q_UNLIKELY(options.timing))
        fprintf(stdout, "[TIMING] %d ms: Read input file\n", options.timer.elapsed());

    fprintf(stdout,
//          "012345678901234567890123456789012345678901234567890123456789012345678901"
            "Generating Android Package\n"
            "  Input file: %s\n"
            "  Output directory: %s\n"
            "  Application binary: %s\n"
            "  Android build platform: %s\n"
            "  Install to device: %s\n",
            qPrintable(options.inputFileName),
            qPrintable(options.outputDirectory),
            qPrintable(options.applicationBinary),
            qPrintable(options.androidPlatform),
            options.installApk
                ? (options.installLocation.isEmpty() ? "Default device" : qPrintable(options.installLocation))
                : "No"
            );

    if (options.build && !options.auxMode) {
        cleanAndroidFiles(options);
        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Cleaned Android file\n", options.timer.elapsed());

        if (!copyAndroidTemplate(options))
            return CannotCopyAndroidTemplate;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Copied Android template\n", options.timer.elapsed());
    }

    for (auto it = options.architectures.constBegin(); it != options.architectures.constEnd(); ++it) {
        options.clear(it.key());

        if (!readDependencies(&options))
            return CannotReadDependencies;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Read dependencies\n", options.timer.elapsed());

        if (!copyQtFiles(&options))
            return CannotCopyQtFiles;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Copied Qt files\n", options.timer.elapsed());

        if (!copyAndroidExtraLibs(&options))
            return CannotCopyAndroidExtraLibs;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Copied extra libs\n", options.timer.elapsed());

        if (!copyAndroidExtraResources(&options))
            return CannotCopyAndroidExtraResources;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Copied extra resources\n", options.timer.elapsed());

        if (!options.auxMode) {
            if (options.deploymentMechanism != Options::Ministro && !copyStdCpp(&options))
                return CannotCopyGnuStl;

            if (Q_UNLIKELY(options.timing))
                fprintf(stdout, "[TIMING] %d ms: Copied GNU STL\n", options.timer.elapsed());
        }

        if (!containsApplicationBinary(&options))
            return CannotFindApplicationBinary;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Checked for application binary\n", options.timer.elapsed());

        if (options.deploymentMechanism != Options::Ministro) {
            if (Q_UNLIKELY(options.timing))
                fprintf(stdout, "[TIMING] %d ms: Bundled Qt libs\n", options.timer.elapsed());
        }
    }

    if (!createRcc(options))
        return CannotCreateRcc;

    if (options.auxMode) {
        if (!updateAndroidFiles(options))
            return CannotUpdateAndroidFiles;
        return 0;
    }


    if (options.build) {
        if (!copyAndroidSources(options))
            return CannotCopyAndroidSources;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Copied android sources\n", options.timer.elapsed());

        if (!updateAndroidFiles(options))
            return CannotUpdateAndroidFiles;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Updated files\n", options.timer.elapsed());

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Created project\n", options.timer.elapsed());

        if (!buildAndroidProject(options))
            return CannotBuildAndroidProject;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Built project\n", options.timer.elapsed());

        if (!options.keyStore.isEmpty() && !signPackage(options))
            return CannotSignPackage;

        if (!options.apkPath.isEmpty() && !copyPackage(options))
            return CannotCopyApk;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %d ms: Signed package\n", options.timer.elapsed());
    }

    if (options.installApk && !installApk(options))
        return CannotInstallApk;

    if (Q_UNLIKELY(options.timing))
        fprintf(stdout, "[TIMING] %d ms: Installed APK\n", options.timer.elapsed());

    fprintf(stdout, "Android package built successfully in %.3f ms.\n", options.timer.elapsed() / 1000.);

    if (options.installApk)
        fprintf(stdout, "  -- It can now be run from the selected device/emulator.\n");

    fprintf(stdout, "  -- File: %s\n", qPrintable(packagePath(options, options.keyStore.isEmpty() ? UnsignedAPK
                                                                                              : SignedAPK)));
    fflush(stdout);
    return 0;
}
