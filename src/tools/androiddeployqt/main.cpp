// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#include <QStandardPaths>
#include <QUuid>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QSettings>
#include <QHash>
#include <QSet>
#include <QMap>

#include <depfile_shared.h>
#include <shellquote_shared.h>

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

using namespace Qt::StringLiterals;

static const bool mustReadOutputAnyway = true; // pclose seems to return the wrong error code unless we read the output

static QStringList dependenciesForDepfile;

FILE *openProcess(const QString &command)
{
#if defined(Q_OS_WIN32)
    QString processedCommand = u'\"' + command + u'\"';
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

struct QtInstallDirectoryWithTriple
{
    QtInstallDirectoryWithTriple(const QString &dir = QString(),
                                 const QString &t = QString(),
                                 const QHash<QString, QString> &dirs = QHash<QString, QString>()
                                ) :
        qtInstallDirectory(dir),
        qtDirectories(dirs),
        triple(t),
        enabled(false)
        {}

    QString qtInstallDirectory;
    QHash<QString, QString> qtDirectories;
    QString triple;
    bool enabled;
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
        , digestAlg("SHA-256"_L1)
        , sigAlg("SHA256withRSA"_L1)
        , internalSf(false)
        , sectionsOnly(false)
        , protectedAuthenticationPath(false)
        , installApk(false)
        , uninstallApk(false)
        , qmlImportScannerBinaryPath()
    {}

    enum DeploymentMechanism
    {
        Bundled,
        Unbundled
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
    bool noRccBundleCleanup = false;
    bool copyDependenciesOnly = false;
    QElapsedTimer timer;

    // External tools
    QString sdkPath;
    QString sdkBuildToolsVersion;
    QString ndkPath;
    QString ndkVersion;
    QString jdkPath;

    // Build paths
    QString qtInstallDirectory;
    QHash<QString, QString> qtDirectories;
    QString qtDataDirectory;
    QString qtLibsDirectory;
    QString qtLibExecsDirectory;
    QString qtPluginsDirectory;
    QString qtQmlDirectory;
    QString qtHostDirectory;
    std::vector<QString> extraPrefixDirs;
    // Unlike 'extraPrefixDirs', the 'extraLibraryDirs' key doesn't expect the 'lib' subfolder
    // when looking for dependencies.
    std::vector<QString> extraLibraryDirs;
    QString androidSourceDirectory;
    QString outputDirectory;
    QString inputFileName;
    QString applicationBinary;
    QString applicationArguments;
    std::vector<QString> rootPaths;
    QString rccBinaryPath;
    QString depFilePath;
    QString buildDirectory;
    QStringList qmlImportPaths;
    QStringList qrcFiles;

    // Versioning
    QString versionName;
    QString versionCode;
    QByteArray minSdkVersion{"23"};
    QByteArray targetSdkVersion{"33"};

    // lib c++ path
    QString stdCppPath;
    QString stdCppName = QStringLiteral("c++_shared");

    // Build information
    QString androidPlatform;
    QHash<QString, QtInstallDirectoryWithTriple> architectures;
    QString currentArchitecture;
    QString toolchainPrefix;
    QString ndkHost;
    bool buildAAB = false;
    bool isZstdCompressionEnabled = false;


    // Package information
    DeploymentMechanism deploymentMechanism;
    QString systemLibsPath;
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
    QString apkPath;

    // Installation information
    bool installApk;
    bool uninstallApk;
    QString installLocation;

    // Per architecture collected information
    void setCurrentQtArchitecture(const QString &arch,
                                  const QString &directory,
                                  const QHash<QString, QString> &directories)
    {
        currentArchitecture = arch;
        qtInstallDirectory = directory;
        qtDataDirectory = directories["qtDataDirectory"_L1];
        qtLibsDirectory = directories["qtLibsDirectory"_L1];
        qtLibExecsDirectory = directories["qtLibExecsDirectory"_L1];
        qtPluginsDirectory = directories["qtPluginsDirectory"_L1];
        qtQmlDirectory = directories["qtQmlDirectory"_L1];
    }
    typedef QPair<QString, QString> BundledFile;
    QHash<QString, QList<BundledFile>> bundledFiles;
    QHash<QString, QList<QtDependency>> qtDependencies;
    QHash<QString, QStringList> localLibs;
    bool usesOpenGL = false;

    // Per package collected information
    QStringList initClasses;
    QStringList permissions;
    QStringList features;

    // Override qml import scanner path
    QString qmlImportScannerBinaryPath;
    bool qmlSkipImportScanning = false;
};

static const QHash<QByteArray, QByteArray> elfArchitectures = {
    {"aarch64", "arm64-v8a"},
    {"arm", "armeabi-v7a"},
    {"i386", "x86"},
    {"x86_64", "x86_64"}
};

bool goodToCopy(const Options *options, const QString &file, QStringList *unmetDependencies);
bool checkCanImportFromRootPaths(const Options *options, const QString &absolutePath,
                                 const QString &moduleUrl);
bool readDependenciesFromElf(Options *options, const QString &fileName,
                             QSet<QString> *usedDependencies, QSet<QString> *remainingDependencies);

QString architectureFromName(const QString &name)
{
    QRegularExpression architecture(QStringLiteral("_(armeabi-v7a|arm64-v8a|x86|x86_64).so$"));
    auto match = architecture.match(name);
    if (!match.hasMatch())
        return {};
    return match.captured(1);
}

static QString execSuffixAppended(QString path)
{
#if defined(Q_OS_WIN32)
    path += ".exe"_L1;
#endif
    return path;
}

static QString batSuffixAppended(QString path)
{
#if defined(Q_OS_WIN32)
    path += ".bat"_L1;
#endif
    return path;
}

QString defaultLibexecDir()
{
#ifdef Q_OS_WIN32
    return "bin"_L1;
#else
    return "libexec"_L1;
#endif
}

static QString llvmReadobjPath(const Options &options)
{
    return execSuffixAppended("%1/toolchains/%2/prebuilt/%3/bin/llvm-readobj"_L1
                              .arg(options.ndkPath,
                                   options.toolchainPrefix,
                                   options.ndkHost));
}

QString fileArchitecture(const Options &options, const QString &path)
{
    auto arch = architectureFromName(path);
    if (!arch.isEmpty())
        return arch;

    QString readElf = llvmReadobjPath(options);
    if (!QFile::exists(readElf)) {
        fprintf(stderr, "Command does not exist: %s\n", qPrintable(readElf));
        return {};
    }

    readElf = "%1 --needed-libs %2"_L1.arg(shellQuote(readElf), shellQuote(path));

    FILE *readElfCommand = openProcess(readElf);
    if (!readElfCommand) {
        fprintf(stderr, "Cannot execute command %s\n", qPrintable(readElf));
        return {};
    }

    char buffer[512];
    while (fgets(buffer, sizeof(buffer), readElfCommand) != nullptr) {
        QByteArray line = QByteArray::fromRawData(buffer, qstrlen(buffer));
        line = line.trimmed();
        if (line.startsWith("Arch: ")) {
            auto it = elfArchitectures.find(line.mid(6));
            pclose(readElfCommand);
            return it != elfArchitectures.constEnd() ? QString::fromLatin1(it.value()) : QString{};
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
                QDir{dst.absolutePath()}.removeRecursively();
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
        if (argument.compare("--output"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.outputDirectory = arguments.at(++i).trimmed();
        } else if (argument.compare("--input"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.inputFileName = arguments.at(++i);
        } else if (argument.compare("--aab"_L1, Qt::CaseInsensitive) == 0) {
            options.buildAAB = true;
            options.build = true;
        } else if (!options.buildAAB && argument.compare("--no-build"_L1, Qt::CaseInsensitive) == 0) {
            options.build = false;
        } else if (argument.compare("--install"_L1, Qt::CaseInsensitive) == 0) {
            options.installApk = true;
            options.uninstallApk = true;
        } else if (argument.compare("--reinstall"_L1, Qt::CaseInsensitive) == 0) {
            options.installApk = true;
            options.uninstallApk = false;
        } else if (argument.compare("--android-platform"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.androidPlatform = arguments.at(++i);
        } else if (argument.compare("--help"_L1, Qt::CaseInsensitive) == 0) {
            options.helpRequested = true;
        } else if (argument.compare("--verbose"_L1, Qt::CaseInsensitive) == 0) {
            options.verbose = true;
        } else if (argument.compare("--deployment"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size()) {
                options.helpRequested = true;
            } else {
                QString deploymentMechanism = arguments.at(++i);
                if (deploymentMechanism.compare("bundled"_L1, Qt::CaseInsensitive) == 0) {
                    options.deploymentMechanism = Options::Bundled;
                } else if (deploymentMechanism.compare("unbundled"_L1,
                                                       Qt::CaseInsensitive) == 0) {
                    options.deploymentMechanism = Options::Unbundled;
                } else {
                    fprintf(stderr, "Unrecognized deployment mechanism: %s\n", qPrintable(deploymentMechanism));
                    options.helpRequested = true;
                }
            }
        } else if (argument.compare("--device"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.installLocation = arguments.at(++i);
        } else if (argument.compare("--release"_L1, Qt::CaseInsensitive) == 0) {
            options.releasePackage = true;
        } else if (argument.compare("--jdk"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.jdkPath = arguments.at(++i);
        } else if (argument.compare("--apk"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.apkPath = arguments.at(++i);
        } else if (argument.compare("--depfile"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.depFilePath = arguments.at(++i);
        } else if (argument.compare("--builddir"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.buildDirectory = arguments.at(++i);
        } else if (argument.compare("--sign"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 2 >= arguments.size()) {
                const QString keyStore = qEnvironmentVariable("QT_ANDROID_KEYSTORE_PATH");
                const QString storeAlias = qEnvironmentVariable("QT_ANDROID_KEYSTORE_ALIAS");
                if (keyStore.isEmpty() || storeAlias.isEmpty()) {
                    options.helpRequested = true;
                    fprintf(stderr, "Package signing path and alias values are not specified.\n");
                } else {
                    fprintf(stdout,
                            "Using package signing path and alias values found from the "
                            "environment variables.\n");
                    options.keyStore = keyStore;
                    options.keyStoreAlias = storeAlias;
                }
            } else if (!arguments.at(i + 1).startsWith("--"_L1) &&
                       !arguments.at(i + 2).startsWith("--"_L1)) {
                options.keyStore = arguments.at(++i);
                options.keyStoreAlias = arguments.at(++i);
            } else {
                options.helpRequested = true;
                fprintf(stderr, "Package signing path and alias values are not "
                                "specified.\n");
            }

            // Do not override if the passwords are provided through arguments
            if (options.keyStorePassword.isEmpty()) {
                fprintf(stdout, "Using package signing store password found from the environment "
                        "variable.\n");
                options.keyStorePassword = qEnvironmentVariable("QT_ANDROID_KEYSTORE_STORE_PASS");
            }
            if (options.keyPass.isEmpty()) {
                fprintf(stdout, "Using package signing key password found from the environment "
                                "variable.\n");
                options.keyPass = qEnvironmentVariable("QT_ANDROID_KEYSTORE_KEY_PASS");
            }
        } else if (argument.compare("--storepass"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.keyStorePassword = arguments.at(++i);
        } else if (argument.compare("--storetype"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.storeType = arguments.at(++i);
        } else if (argument.compare("--keypass"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.keyPass = arguments.at(++i);
        } else if (argument.compare("--sigfile"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.sigFile = arguments.at(++i);
        } else if (argument.compare("--digestalg"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.digestAlg = arguments.at(++i);
        } else if (argument.compare("--sigalg"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.sigAlg = arguments.at(++i);
        } else if (argument.compare("--tsa"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.tsaUrl = arguments.at(++i);
        } else if (argument.compare("--tsacert"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                options.helpRequested = true;
            else
                options.tsaCert = arguments.at(++i);
        } else if (argument.compare("--internalsf"_L1, Qt::CaseInsensitive) == 0) {
            options.internalSf = true;
        } else if (argument.compare("--sectionsonly"_L1, Qt::CaseInsensitive) == 0) {
            options.sectionsOnly = true;
        } else if (argument.compare("--protected"_L1, Qt::CaseInsensitive) == 0) {
            options.protectedAuthenticationPath = true;
        } else if (argument.compare("--aux-mode"_L1, Qt::CaseInsensitive) == 0) {
            options.auxMode = true;
        } else if (argument.compare("--qml-importscanner-binary"_L1, Qt::CaseInsensitive) == 0) {
            options.qmlImportScannerBinaryPath = arguments.at(++i).trimmed();
        } else if (argument.compare("--no-rcc-bundle-cleanup"_L1,
                                    Qt::CaseInsensitive) == 0) {
            options.noRccBundleCleanup = true;
        } else if (argument.compare("--copy-dependencies-only"_L1,
                                    Qt::CaseInsensitive) == 0) {
            options.copyDependenciesOnly = true;
        }
    }

    if (options.buildDirectory.isEmpty() && !options.depFilePath.isEmpty())
        options.helpRequested = true;

    if (options.inputFileName.isEmpty())
        options.inputFileName = "android-%1-deployment-settings.json"_L1.arg(QDir::current().dirName());

    options.timing = qEnvironmentVariableIsSet("ANDROIDDEPLOYQT_TIMING_OUTPUT");

    if (!QDir::current().mkpath(options.outputDirectory)) {
        fprintf(stderr, "Invalid output directory: %s\n", qPrintable(options.outputDirectory));
        options.outputDirectory.clear();
    } else {
        options.outputDirectory = QFileInfo(options.outputDirectory).canonicalFilePath();
        if (!options.outputDirectory.endsWith(u'/'))
            options.outputDirectory += u'/';
    }

    return options;
}

void printHelp()
{
    fprintf(stderr, R"(
Syntax: androiddeployqt --output <destination> [options]

Creates an Android package in the build directory <destination> and
builds it into an .apk file.

Optional arguments:
    --input <inputfile>: Reads <inputfile> for options generated by
       qmake. A default file name based on the current working
       directory will be used if nothing else is specified.

    --deployment <mechanism>: Supported deployment mechanisms:
       bundled (default): Includes Qt files in stand-alone package.
       unbundled: Assumes native libraries are present on the device
       and does not include them in the APK.

    --aab: Build an Android App Bundle.

    --no-build: Do not build the package, it is useful to just install
       a package previously built.

    --install: Installs apk to device/emulator. By default this step is
       not taken. If the application has previously been installed on
       the device, it will be uninstalled first.

    --reinstall: Installs apk to device/emulator. By default this step
       is not taken. If the application has previously been installed on
       the device, it will be overwritten, but its data will be left
       intact.

    --device [device ID]: Use specified device for deployment. Default
       is the device selected by default by adb.

    --android-platform <platform>: Builds against the given android
       platform. By default, the highest available version will be
       used.

    --release: Builds a package ready for release. By default, the
       package will be signed with a debug key.

    --sign <url/to/keystore> <alias>: Signs the package with the
       specified keystore, alias and store password.
       Optional arguments for use with signing:
         --storepass <password>: Keystore password.
         --storetype <type>: Keystore type.
         --keypass <password>: Password for private key (if different
           from keystore password.)
         --sigfile <file>: Name of .SF/.DSA file.
         --digestalg <name>: Name of digest algorithm. Default is
           "SHA1".
         --sigalg <name>: Name of signature algorithm. Default is
           "SHA1withRSA".
         --tsa <url>: Location of the Time Stamping Authority.
         --tsacert <alias>: Public key certificate for TSA.
         --internalsf: Include the .SF file inside the signature block.
         --sectionsonly: Don't compute hash of entire manifest.
         --protected: Keystore has protected authentication path.
         --jarsigner: Deprecated, ignored.

       NOTE: To conceal the keystore information, the environment variables
         QT_ANDROID_KEYSTORE_PATH, and QT_ANDROID_KEYSTORE_ALIAS are used to
         set the values keysotore and alias respectively.
         Also the environment variables QT_ANDROID_KEYSTORE_STORE_PASS,
         and QT_ANDROID_KEYSTORE_KEY_PASS are used to set the store and key
         passwords respectively. This option needs only the --sign parameter.

    --jdk <path/to/jdk>: Used to find the jarsigner tool when used
       in combination with the --release argument. By default,
       an attempt is made to detect the tool using the JAVA_HOME and
       PATH environment variables, in that order.

    --qml-import-paths: Specify additional search paths for QML
       imports.

    --verbose: Prints out information during processing.

    --no-generated-assets-cache: Do not pregenerate the entry list for
       the assets file engine.

    --aux-mode: Operate in auxiliary mode. This will only copy the
       dependencies into the build directory and update the XML templates.
       The project will not be built or installed.

    --apk <path/where/to/copy/the/apk>: Path where to copy the built apk.

    --qml-importscanner-binary <path/to/qmlimportscanner>: Override the
       default qmlimportscanner binary path. By default the
       qmlimportscanner binary is located using the Qt directory
       specified in the input file.

    --depfile <path/to/depfile>: Output a dependency file.

    --builddir <path/to/build/directory>: build directory. Necessary when
       generating a depfile because ninja requires relative paths.

    --no-rcc-bundle-cleanup: skip cleaning rcc bundle directory after
       running androiddeployqt. This option simplifies debugging of
       the resource bundle content, but it should not be used when deploying
       a project, since it litters the 'assets' directory.

    --copy-dependencies-only: resolve application dependencies and stop
       deploying process after all libraries and resources that the
       application depends on have been copied.

    --help: Displays this information.
)");
}

// Since strings compared will all start with the same letters,
// sorting by length and then alphabetically within each length
// gives the natural order.
bool quasiLexicographicalReverseLessThan(const QFileInfo &fi1, const QFileInfo &fi2)
{
    QString s1 = fi1.baseName();
    QString s2 = fi2.baseName();

    if (s1.size() == s2.size())
        return s1 > s2;
    else
        return s1.size() > s2.size();
}

// Files which contain templates that need to be overwritten by build data should be overwritten every
// time.
bool alwaysOverwritableFile(const QString &fileName)
{
    return (fileName.endsWith("/res/values/libs.xml"_L1)
            || fileName.endsWith("/AndroidManifest.xml"_L1)
            || fileName.endsWith("/res/values/strings.xml"_L1)
            || fileName.endsWith("/src/org/qtproject/qt/android/bindings/QtActivity.java"_L1));
}


bool copyFileIfNewer(const QString &sourceFileName,
                     const QString &destinationFileName,
                     const Options &options,
                     bool forceOverwrite = false)
{
    dependenciesForDepfile << sourceFileName;
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
    auto isLegalChar = [] (QChar c) -> bool {
        ushort ch = c.unicode();
        return (ch >= '0' && ch <= '9') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= 'a' && ch <= 'z') ||
                ch == '.';
    };
    for (QChar &c : packageName) {
        if (!isLegalChar(c))
            c = u'_';
    }

    static QStringList keywords;
    if (keywords.isEmpty()) {
        keywords << "abstract"_L1 << "continue"_L1 << "for"_L1
                 << "new"_L1 << "switch"_L1 << "assert"_L1
                 << "default"_L1 << "if"_L1 << "package"_L1
                 << "synchronized"_L1 << "boolean"_L1 << "do"_L1
                 << "goto"_L1 << "private"_L1 << "this"_L1
                 << "break"_L1 << "double"_L1 << "implements"_L1
                 << "protected"_L1 << "throw"_L1 << "byte"_L1
                 << "else"_L1 << "import"_L1 << "public"_L1
                 << "throws"_L1 << "case"_L1 << "enum"_L1
                 << "instanceof"_L1 << "return"_L1 << "transient"_L1
                 << "catch"_L1 << "extends"_L1 << "int"_L1
                 << "short"_L1 << "try"_L1 << "char"_L1
                 << "final"_L1 << "interface"_L1 << "static"_L1
                 << "void"_L1 << "class"_L1 << "finally"_L1
                 << "long"_L1 << "strictfp"_L1 << "volatile"_L1
                 << "const"_L1 << "float"_L1 << "native"_L1
                 << "super"_L1 << "while"_L1;
    }

    // No keywords
    qsizetype index = -1;
    while (index < packageName.size()) {
        qsizetype next = packageName.indexOf(u'.', index + 1);
        if (next == -1)
            next = packageName.size();
        QString word = packageName.mid(index + 1, next - index - 1);
        if (!word.isEmpty()) {
            QChar c = word[0];
            if ((c >= u'0' && c <= u'9') || c == u'_') {
                packageName.insert(index + 1, u'a');
                index = next + 1;
                continue;
            }
        }
        if (keywords.contains(word)) {
            packageName.insert(next, "_"_L1);
            index = next + 1;
        } else {
            index = next;
        }
    }

    return packageName;
}

QString detectLatestAndroidPlatform(const QString &sdkPath)
{
    QDir dir(sdkPath + "/platforms"_L1);
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
            if (reader.isStartElement() && reader.name() == "manifest"_L1)
                return cleanPackageName(reader.attributes().value("package"_L1).toString());
        }
    }
    return {};
}

bool parseCmakeBoolean(const QJsonValue &value)
{
    const QString stringValue = value.toString();
    return (stringValue.compare(QString::fromUtf8("true"), Qt::CaseInsensitive)
                 || stringValue.compare(QString::fromUtf8("on"), Qt::CaseInsensitive)
                 || stringValue.compare(QString::fromUtf8("yes"), Qt::CaseInsensitive)
                 || stringValue.compare(QString::fromUtf8("y"), Qt::CaseInsensitive)
                 || stringValue.toInt() > 0);
}

bool readInputFileDirectory(Options *options, QJsonObject &jsonObject, const QString keyName)
{
    const QJsonValue qtDirectory = jsonObject.value(keyName);
    if (qtDirectory.isUndefined()) {
        for (auto it = options->architectures.constBegin(); it != options->architectures.constEnd(); ++it) {
            if (keyName == "qtDataDirectory"_L1) {
                    options->architectures[it.key()].qtDirectories[keyName] = "."_L1;
                    break;
            } else if (keyName == "qtLibsDirectory"_L1) {
                    options->architectures[it.key()].qtDirectories[keyName] = "lib"_L1;
                    break;
            } else if (keyName == "qtLibExecsDirectory"_L1) {
                    options->architectures[it.key()].qtDirectories[keyName] = defaultLibexecDir();
                    break;
            } else if (keyName == "qtPluginsDirectory"_L1) {
                    options->architectures[it.key()].qtDirectories[keyName] = "plugins"_L1;
                    break;
            } else if (keyName == "qtQmlDirectory"_L1) {
                    options->architectures[it.key()].qtDirectories[keyName] = "qml"_L1;
                    break;
            }
        }
        return true;
    }

    if (qtDirectory.isObject()) {
        const QJsonObject object = qtDirectory.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            if (it.value().isUndefined()) {
                    fprintf(stderr,
                            "Invalid '%s' record in deployment settings: %s\n",
                            qPrintable(keyName),
                            qPrintable(it.value().toString()));
                    return false;
            }
            if (it.value().isNull())
                continue;
            if (!options->architectures.contains(it.key())) {
                fprintf(stderr, "Architecture %s unknown (%s).", qPrintable(it.key()),
                        qPrintable(options->architectures.keys().join(u',')));
                return false;
            }
            options->architectures[it.key()].qtDirectories[keyName] = it.value().toString();
        }
    } else if (qtDirectory.isString()) {
        // Format for Qt < 6 or when using the tool with Qt >= 6 but in single arch.
        // We assume Qt > 5.14 where all architectures are in the same directory.
        const QString directory = qtDirectory.toString();
        options->architectures["arm64-v8a"_L1].qtDirectories[keyName] = directory;
        options->architectures["armeabi-v7a"_L1].qtDirectories[keyName] = directory;
        options->architectures["x86"_L1].qtDirectories[keyName] = directory;
        options->architectures["x86_64"_L1].qtDirectories[keyName] = directory;
    } else {
        fprintf(stderr, "Invalid format for %s in json file %s.\n",
                qPrintable(keyName), qPrintable(options->inputFileName));
        return false;
    }
    return true;
}

bool readInputFile(Options *options)
{
    QFile file(options->inputFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        fprintf(stderr, "Cannot read from input file: %s\n", qPrintable(options->inputFileName));
        return false;
    }
    dependenciesForDepfile << options->inputFileName;

    QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll());
    if (jsonDocument.isNull()) {
        fprintf(stderr, "Invalid json file: %s\n", qPrintable(options->inputFileName));
        return false;
    }

    QJsonObject jsonObject = jsonDocument.object();

    {
        QJsonValue sdkPath = jsonObject.value("sdk"_L1);
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
            if (!QDir(options->sdkPath + "/platforms/"_L1 + options->androidPlatform).exists()) {
                fprintf(stderr, "Warning: Android platform '%s' does not exist in SDK.\n",
                        qPrintable(options->androidPlatform));
            }
        }
    }

    {

        const QJsonValue value = jsonObject.value("sdkBuildToolsRevision"_L1);
        if (!value.isUndefined())
            options->sdkBuildToolsVersion = value.toString();
    }

    {
        const QJsonValue qtInstallDirectory = jsonObject.value("qt"_L1);
        if (qtInstallDirectory.isUndefined()) {
            fprintf(stderr, "No Qt directory in json file %s\n", qPrintable(options->inputFileName));
            return false;
        }

        if (qtInstallDirectory.isObject()) {
            const QJsonObject object = qtInstallDirectory.toObject();
            for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
                if (it.value().isUndefined()) {
                    fprintf(stderr,
                            "Invalid 'qt' record in deployment settings: %s\n",
                            qPrintable(it.value().toString()));
                    return false;
                }
                if (it.value().isNull())
                    continue;
                options->architectures.insert(it.key(),
                                              QtInstallDirectoryWithTriple(it.value().toString()));
            }
        } else if (qtInstallDirectory.isString()) {
            // Format for Qt < 6 or when using the tool with Qt >= 6 but in single arch.
            // We assume Qt > 5.14 where all architectures are in the same directory.
            const QString directory = qtInstallDirectory.toString();
            QtInstallDirectoryWithTriple qtInstallDirectoryWithTriple(directory);
            options->architectures.insert("arm64-v8a"_L1, qtInstallDirectoryWithTriple);
            options->architectures.insert("armeabi-v7a"_L1, qtInstallDirectoryWithTriple);
            options->architectures.insert("x86"_L1, qtInstallDirectoryWithTriple);
            options->architectures.insert("x86_64"_L1, qtInstallDirectoryWithTriple);
            // In Qt < 6 rcc and qmlimportscanner are installed in the host and install directories
            // In Qt >= 6 rcc and qmlimportscanner are only installed in the host directory
            // So setting the "qtHostDir" is not necessary with Qt < 6.
            options->qtHostDirectory = directory;
        } else {
            fprintf(stderr, "Invalid format for Qt install prefixes in json file %s.\n",
                    qPrintable(options->inputFileName));
            return false;
        }
    }

    if (!readInputFileDirectory(options, jsonObject, "qtDataDirectory"_L1) ||
        !readInputFileDirectory(options, jsonObject, "qtLibsDirectory"_L1) ||
        !readInputFileDirectory(options, jsonObject, "qtLibExecsDirectory"_L1) ||
        !readInputFileDirectory(options, jsonObject, "qtPluginsDirectory"_L1) ||
        !readInputFileDirectory(options, jsonObject, "qtQmlDirectory"_L1))
        return false;

    {
        const QJsonValue qtHostDirectory = jsonObject.value("qtHostDir"_L1);
        if (!qtHostDirectory.isUndefined()) {
            if (qtHostDirectory.isString()) {
                options->qtHostDirectory = qtHostDirectory.toString();
            } else {
                fprintf(stderr, "Invalid format for Qt host directory in json file %s.\n",
                        qPrintable(options->inputFileName));
                return false;
            }
        }
    }

    {
        const auto extraPrefixDirs = jsonObject.value("extraPrefixDirs"_L1).toArray();
        options->extraPrefixDirs.reserve(extraPrefixDirs.size());
        for (const QJsonValue prefix : extraPrefixDirs) {
            options->extraPrefixDirs.push_back(prefix.toString());
        }
    }

    {
        const auto extraLibraryDirs = jsonObject.value("extraLibraryDirs"_L1).toArray();
        options->extraLibraryDirs.reserve(extraLibraryDirs.size());
        for (const QJsonValue path : extraLibraryDirs) {
            options->extraLibraryDirs.push_back(path.toString());
        }
    }

    {
        const QJsonValue androidSourcesDirectory = jsonObject.value("android-package-source-directory"_L1);
        if (!androidSourcesDirectory.isUndefined())
            options->androidSourceDirectory = androidSourcesDirectory.toString();
    }

    {
        const QJsonValue applicationArguments = jsonObject.value("android-application-arguments"_L1);
        if (!applicationArguments.isUndefined())
            options->applicationArguments = applicationArguments.toString();
        else
            options->applicationArguments = QStringLiteral("");
    }

    {
        const QJsonValue androidVersionName = jsonObject.value("android-version-name"_L1);
        if (!androidVersionName.isUndefined())
            options->versionName = androidVersionName.toString();
        else
            options->versionName = QStringLiteral("1.0");
    }

    {
        const QJsonValue androidVersionCode = jsonObject.value("android-version-code"_L1);
        if (!androidVersionCode.isUndefined())
            options->versionCode = androidVersionCode.toString();
        else
            options->versionCode = QStringLiteral("1");
    }

    {
        const QJsonValue ver = jsonObject.value("android-min-sdk-version"_L1);
        if (!ver.isUndefined())
            options->minSdkVersion = ver.toString().toUtf8();
    }

    {
        const QJsonValue ver = jsonObject.value("android-target-sdk-version"_L1);
        if (!ver.isUndefined())
            options->targetSdkVersion = ver.toString().toUtf8();
    }

    {
        const QJsonObject targetArchitectures = jsonObject.value("architectures"_L1).toObject();
        if (targetArchitectures.isEmpty()) {
            fprintf(stderr, "No target architecture defined in json file.\n");
            return false;
        }
        for (auto it = targetArchitectures.constBegin(); it != targetArchitectures.constEnd(); ++it) {
            if (it.value().isUndefined()) {
                fprintf(stderr, "Invalid architecture.\n");
                return false;
            }
            if (it.value().isNull())
                continue;
            if (!options->architectures.contains(it.key())) {
                fprintf(stderr, "Architecture %s unknown (%s).", qPrintable(it.key()),
                        qPrintable(options->architectures.keys().join(u',')));
                return false;
            }
            options->architectures[it.key()].triple = it.value().toString();
            options->architectures[it.key()].enabled = true;
        }
    }

    {
        const QJsonValue ndk = jsonObject.value("ndk"_L1);
        if (ndk.isUndefined()) {
            fprintf(stderr, "No NDK path defined in json file.\n");
            return false;
        }
        options->ndkPath = ndk.toString();
        const QString ndkPropertiesPath = options->ndkPath + QStringLiteral("/source.properties");
        const QSettings settings(ndkPropertiesPath, QSettings::IniFormat);
        const QString ndkVersion = settings.value(QStringLiteral("Pkg.Revision")).toString();
        if (ndkVersion.isEmpty()) {
            fprintf(stderr, "Couldn't retrieve the NDK version from \"%s\".\n",
                    qPrintable(ndkPropertiesPath));
            return false;
        }
        options->ndkVersion = ndkVersion;
    }

    {
        const QJsonValue toolchainPrefix = jsonObject.value("toolchain-prefix"_L1);
        if (toolchainPrefix.isUndefined()) {
            fprintf(stderr, "No toolchain prefix defined in json file.\n");
            return false;
        }
        options->toolchainPrefix = toolchainPrefix.toString();
    }

    {
        const QJsonValue ndkHost = jsonObject.value("ndk-host"_L1);
        if (ndkHost.isUndefined()) {
            fprintf(stderr, "No NDK host defined in json file.\n");
            return false;
        }
        options->ndkHost = ndkHost.toString();
    }

    {
        const QJsonValue extraLibs = jsonObject.value("android-extra-libs"_L1);
        if (!extraLibs.isUndefined())
            options->extraLibs = extraLibs.toString().split(u',', Qt::SkipEmptyParts);
    }

    {
        const QJsonValue qmlSkipImportScanning = jsonObject.value("qml-skip-import-scanning"_L1);
        if (!qmlSkipImportScanning.isUndefined())
            options->qmlSkipImportScanning = qmlSkipImportScanning.toBool();
    }

    {
        const QJsonValue extraPlugins = jsonObject.value("android-extra-plugins"_L1);
        if (!extraPlugins.isUndefined())
            options->extraPlugins = extraPlugins.toString().split(u',');
    }

    {
        const QJsonValue systemLibsPath =
                jsonObject.value("android-system-libs-prefix"_L1);
        if (!systemLibsPath.isUndefined())
            options->systemLibsPath = systemLibsPath.toString();
    }

    {
        const QJsonValue noDeploy = jsonObject.value("android-no-deploy-qt-libs"_L1);
        if (!noDeploy.isUndefined()) {
            bool useUnbundled = parseCmakeBoolean(noDeploy);
            options->deploymentMechanism = useUnbundled ? Options::Unbundled :
                                                          Options::Bundled;
        }
    }

    {
        const QJsonValue stdcppPath = jsonObject.value("stdcpp-path"_L1);
        if (stdcppPath.isUndefined()) {
            fprintf(stderr, "No stdcpp-path defined in json file.\n");
            return false;
        }
        options->stdCppPath = stdcppPath.toString();
    }

    {
        const QJsonValue qmlRootPath = jsonObject.value("qml-root-path"_L1);
        if (qmlRootPath.isString()) {
            options->rootPaths.push_back(qmlRootPath.toString());
        } else if (qmlRootPath.isArray()) {
            auto qmlRootPaths = qmlRootPath.toArray();
            for (auto path : qmlRootPaths) {
                if (path.isString())
                    options->rootPaths.push_back(path.toString());
            }
        } else {
            options->rootPaths.push_back(QFileInfo(options->inputFileName).absolutePath());
        }
    }

    {
        const QJsonValue qmlImportPaths = jsonObject.value("qml-import-paths"_L1);
        if (!qmlImportPaths.isUndefined())
            options->qmlImportPaths = qmlImportPaths.toString().split(u',');
    }

    {
        const QJsonValue qmlImportScannerBinaryPath = jsonObject.value("qml-importscanner-binary"_L1);
        if (!qmlImportScannerBinaryPath.isUndefined())
            options->qmlImportScannerBinaryPath = qmlImportScannerBinaryPath.toString();
    }

    {
        const QJsonValue rccBinaryPath = jsonObject.value("rcc-binary"_L1);
        if (!rccBinaryPath.isUndefined())
            options->rccBinaryPath = rccBinaryPath.toString();
    }

    {
        const QJsonValue applicationBinary = jsonObject.value("application-binary"_L1);
        if (applicationBinary.isUndefined()) {
            fprintf(stderr, "No application binary defined in json file.\n");
            return false;
        }
        options->applicationBinary = applicationBinary.toString();
        if (options->build) {
            for (auto it = options->architectures.constBegin(); it != options->architectures.constEnd(); ++it) {
                if (!it->enabled)
                    continue;
                auto appBinaryPath = "%1/libs/%2/lib%3_%2.so"_L1.arg(options->outputDirectory, it.key(), options->applicationBinary);
                if (!QFile::exists(appBinaryPath)) {
                    fprintf(stderr, "Cannot find application binary in build dir %s.\n", qPrintable(appBinaryPath));
                    return false;
                }
            }
        }
    }

    {
        const QJsonValue deploymentDependencies = jsonObject.value("deployment-dependencies"_L1);
        if (!deploymentDependencies.isUndefined()) {
            QString deploymentDependenciesString = deploymentDependencies.toString();
            const auto dependencies = QStringView{deploymentDependenciesString}.split(u',');
            for (const auto &dependency : dependencies) {
                QString path = options->qtInstallDirectory + QChar::fromLatin1('/');
                path += dependency;
                if (QFileInfo(path).isDir()) {
                    QDirIterator iterator(path, QDirIterator::Subdirectories);
                    while (iterator.hasNext()) {
                        iterator.next();
                        if (iterator.fileInfo().isFile()) {
                            QString subPath = iterator.filePath();
                            auto arch = fileArchitecture(*options, subPath);
                            if (!arch.isEmpty()) {
                                options->qtDependencies[arch].append(QtDependency(subPath.mid(options->qtInstallDirectory.size() + 1),
                                                                                  subPath));
                            } else if (options->verbose) {
                                fprintf(stderr, "Skipping \"%s\", unknown architecture\n", qPrintable(subPath));
                                fflush(stderr);
                            }
                        }
                    }
                } else {
                    auto qtDependency = [options](const QStringView &dependency,
                                                  const QString &arch) {
                        const auto installDir = options->architectures[arch].qtInstallDirectory;
                        const auto absolutePath = "%1/%2"_L1.arg(installDir, dependency.toString());
                        return QtDependency(dependency.toString(), absolutePath);
                    };

                    if (dependency.endsWith(QLatin1String(".so"))) {
                        auto arch = fileArchitecture(*options, path);
                        if (!arch.isEmpty()) {
                            options->qtDependencies[arch].append(qtDependency(dependency, arch));
                        } else if (options->verbose) {
                            fprintf(stderr, "Skipping \"%s\", unknown architecture\n", qPrintable(path));
                            fflush(stderr);
                        }
                    } else {
                        for (auto arch : options->architectures.keys())
                            options->qtDependencies[arch].append(qtDependency(dependency, arch));
                    }
                }
            }
        }
    }
    {
        const QJsonValue qrcFiles = jsonObject.value("qrcFiles"_L1);
        options->qrcFiles = qrcFiles.toString().split(u',', Qt::SkipEmptyParts);
    }
    {
        const QJsonValue zstdCompressionFlag = jsonObject.value("zstdCompression"_L1);
        if (zstdCompressionFlag.isBool()) {
            options->isZstdCompressionEnabled = zstdCompressionFlag.toBool();
        }
    }
    options->packageName = packageNameFromAndroidManifest(options->androidSourceDirectory + "/AndroidManifest.xml"_L1);
    if (options->packageName.isEmpty())
        options->packageName = cleanPackageName("org.qtproject.example.%1"_L1.arg(options->applicationBinary));

    return true;
}

bool isDeployment(const Options *options, Options::DeploymentMechanism deployment)
{
    return options->deploymentMechanism == deployment;
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

            if (!copyFiles(dir, QDir(destinationDirectory.path() + u'/' + dir.dirName()), options, forceOverwrite))
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
        if (dir.fileName() != "libs"_L1)
            deleteMissingFiles(options, dir.absoluteFilePath(), QDir(dstDir + dir.fileName()));
    }
}

void cleanAndroidFiles(const Options &options)
{
    if (!options.androidSourceDirectory.isEmpty())
        cleanTopFolders(options, QDir(options.androidSourceDirectory), options.outputDirectory);

    cleanTopFolders(options,
                    QDir(options.qtInstallDirectory + u'/' +
                         options.qtDataDirectory + "/src/android/templates"_L1),
                    options.outputDirectory);
}

bool copyAndroidTemplate(const Options &options, const QString &androidTemplate, const QString &outDirPrefix = QString())
{
    QDir sourceDirectory(options.qtInstallDirectory + u'/' + options.qtDataDirectory + androidTemplate);
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
    QDir sourceDirectory(options.qtInstallDirectory + u'/' +
                         options.qtDataDirectory + "/src/3rdparty/gradle"_L1);
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

    if (!copyAndroidTemplate(options, "/src/android/templates"_L1))
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

    if (options->verbose) {
        switch (options->deploymentMechanism) {
        case Options::Bundled:
            fprintf(stdout, "Copying %zd external libraries to package.\n", size_t(options->extraLibs.size()));
            break;
        case Options::Unbundled:
            fprintf(stdout, "Skip copying of external libraries.\n");
            break;
        };
    }

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
        if (!extraLibInfo.fileName().startsWith("lib"_L1) || extraLibInfo.suffix() != "so"_L1) {
            fprintf(stderr, "The file name of external library %s must begin with \"lib\" and end with the suffix \".so\".\n",
                    qPrintable(extraLib));
            return false;
        }
        QString destinationFile(options->outputDirectory
                                + "/libs/"_L1
                                + options->currentArchitecture
                                + u'/'
                                + extraLibInfo.fileName());

        if (isDeployment(options, Options::Bundled)
                && !copyFileIfNewer(extraLib, destinationFile, *options)) {
            return false;
        }
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
        fprintf(stdout, "Copying %zd external resources to package.\n", size_t(options->extraPlugins.size()));

    for (const QString &extraResource : options->extraPlugins) {
        QFileInfo extraResourceInfo(extraResource);
        if (!extraResourceInfo.exists() || !extraResourceInfo.isDir()) {
            fprintf(stderr, "External resource %s does not exist or not a correct directory!\n", qPrintable(extraResource));
            return false;
        }

        QDir resourceDir(extraResource);
        QString assetsDir = options->outputDirectory + "/assets/"_L1 +
                            resourceDir.dirName() + u'/';
        QString libsDir = options->outputDirectory + "/libs/"_L1 + options->currentArchitecture + u'/';

        const QStringList files = allFilesInside(resourceDir, resourceDir);
        for (const QString &resourceFile : files) {
            QString originFile(resourceDir.filePath(resourceFile));
            QString destinationFile;
            if (!resourceFile.endsWith(".so"_L1)) {
                destinationFile = assetsDir + resourceFile;
            } else {
                if (isDeployment(options, Options::Unbundled)
                                 || !checkArchitecture(*options, originFile)) {
                    continue;
                }
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
                contents.replace(index, it.key().size(), it.value().toUtf8());
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

    QString fileName = options->outputDirectory + "/res/values/libs.xml"_L1;
    if (!QFile::exists(fileName)) {
        fprintf(stderr, "Cannot find %s in prepared packaged. This file is required.\n", qPrintable(fileName));
        return false;
    }

    QString qtLibs;
    QString allLocalLibs;
    QString extraLibs;

    for (auto it = options->architectures.constBegin(); it != options->architectures.constEnd(); ++it) {
        if (!it->enabled)
            continue;

        qtLibs += "        <item>%1;%2</item>\n"_L1.arg(it.key(), options->stdCppName);
        for (const Options::BundledFile &bundledFile : options->bundledFiles[it.key()]) {
            if (bundledFile.second.startsWith("lib/lib"_L1)) {
                if (!bundledFile.second.endsWith(".so"_L1)) {
                    fprintf(stderr,
                            "The bundled library %s doesn't end with .so. Android only supports "
                            "versionless libraries ending with the .so suffix.\n",
                            qPrintable(bundledFile.second));
                    return false;
                }
                QString s = bundledFile.second.mid(sizeof("lib/lib") - 1);
                s.chop(sizeof(".so") - 1);
                qtLibs += "        <item>%1;%2</item>\n"_L1.arg(it.key(), s);
            }
        }

        if (!options->archExtraLibs[it.key()].isEmpty()) {
            for (const QString &extraLib : options->archExtraLibs[it.key()]) {
                QFileInfo extraLibInfo(extraLib);
                if (extraLibInfo.fileName().startsWith("lib"_L1)) {
                    if (!extraLibInfo.fileName().endsWith(".so"_L1)) {
                        fprintf(stderr,
                                "The library %s doesn't end with .so. Android only supports "
                                "versionless libraries ending with the .so suffix.\n",
                                qPrintable(extraLibInfo.fileName()));
                        return false;
                    }
                    QString name = extraLibInfo.fileName().mid(sizeof("lib") - 1);
                    name.chop(sizeof(".so") - 1);
                    extraLibs += "        <item>%1;%2</item>\n"_L1.arg(it.key(), name);
                }
            }
        }

        QStringList localLibs;
        localLibs = options->localLibs[it.key()];
        // If .pro file overrides dependency detection, we need to see which platform plugin they picked
        if (localLibs.isEmpty()) {
            QString plugin;
            for (const QtDependency &qtDependency : options->qtDependencies[it.key()]) {
                if (qtDependency.relativePath.contains("libplugins_platforms_qtforandroid_"_L1))
                    plugin = qtDependency.relativePath;

                if (qtDependency.relativePath.contains(
                            QString::asprintf("libQt%dOpenGL", QT_VERSION_MAJOR))
                    || qtDependency.relativePath.contains(
                            QString::asprintf("libQt%dQuick", QT_VERSION_MAJOR))) {
                    options->usesOpenGL |= true;
                }
            }

            if (plugin.isEmpty()) {
                fflush(stdout);
                fprintf(stderr, "No platform plugin (libplugins_platforms_qtforandroid.so) included"
                                " in the deployment. Make sure the app links to Qt Gui library.\n");
                fflush(stderr);
                return false;
            }

            localLibs.append(plugin);
            if (options->verbose)
                fprintf(stdout, "  -- Using platform plugin %s\n", qPrintable(plugin));
        }

        // remove all paths
        for (auto &lib : localLibs) {
            if (lib.endsWith(".so"_L1))
                lib = lib.mid(lib.lastIndexOf(u'/') + 1);
        }
        allLocalLibs += "        <item>%1;%2</item>\n"_L1.arg(it.key(), localLibs.join(u':'));
    }

    options->initClasses.removeDuplicates();

    QHash<QString, QString> replacements;
    replacements[QStringLiteral("<!-- %%INSERT_QT_LIBS%% -->")] += qtLibs.trimmed();
    replacements[QStringLiteral("<!-- %%INSERT_LOCAL_LIBS%% -->")] = allLocalLibs.trimmed();
    replacements[QStringLiteral("<!-- %%INSERT_EXTRA_LIBS%% -->")] = extraLibs.trimmed();
    const QString initClasses = options->initClasses.join(u':');
    replacements[QStringLiteral("<!-- %%INSERT_INIT_CLASSES%% -->")] = initClasses;

    // Set BUNDLE_LOCAL_QT_LIBS based on the deployment used
    replacements[QStringLiteral("<!-- %%BUNDLE_LOCAL_QT_LIBS%% -->")]
            = isDeployment(options, Options::Unbundled) ? "0"_L1 : "1"_L1;
    replacements[QStringLiteral("<!-- %%USE_LOCAL_QT_LIBS%% -->")] = "1"_L1;
    replacements[QStringLiteral("<!-- %%SYSTEM_LIBS_PREFIX%% -->")] =
            isDeployment(options, Options::Unbundled) ? options->systemLibsPath : QStringLiteral("");

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

    QString fileName = options.outputDirectory + "/res/values/strings.xml"_L1;
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

    QHash<QString, QString> replacements;
    replacements[QStringLiteral("-- %%INSERT_APP_NAME%% --")] = options.applicationBinary;
    replacements[QStringLiteral("-- %%INSERT_APP_ARGUMENTS%% --")] = options.applicationArguments;
    replacements[QStringLiteral("-- %%INSERT_APP_LIB_NAME%% --")] = options.applicationBinary;
    replacements[QStringLiteral("-- %%INSERT_VERSION_NAME%% --")] = options.versionName;
    replacements[QStringLiteral("-- %%INSERT_VERSION_CODE%% --")] = options.versionCode;
    replacements[QStringLiteral("package=\"org.qtproject.example\"")] = "package=\"%1\""_L1.arg(options.packageName);

    QString permissions;
    for (const QString &permission : std::as_const(options.permissions))
        permissions += "    <uses-permission android:name=\"%1\" />\n"_L1.arg(permission);
    replacements[QStringLiteral("<!-- %%INSERT_PERMISSIONS -->")] = permissions.trimmed();

    QString features;
    for (const QString &feature : std::as_const(options.features))
        features += "    <uses-feature android:name=\"%1\" android:required=\"false\" />\n"_L1.arg(feature);
    if (options.usesOpenGL)
        features += "    <uses-feature android:glEsVersion=\"0x00020000\" android:required=\"true\" />"_L1;

    replacements[QStringLiteral("<!-- %%INSERT_FEATURES -->")] = features.trimmed();

    QString androidManifestPath = options.outputDirectory + "/AndroidManifest.xml"_L1;
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
                if (reader.name() == "manifest"_L1) {
                    if (!reader.attributes().hasAttribute("package"_L1)) {
                        fprintf(stderr, "Invalid android manifest file: %s\n", qPrintable(androidManifestPath));
                        return false;
                    }
                    options.packageName = reader.attributes().value("package"_L1).toString();
                } else if (reader.name() == "uses-sdk"_L1) {
                    if (reader.attributes().hasAttribute("android:minSdkVersion"_L1))
                        if (reader.attributes().value("android:minSdkVersion"_L1).toInt() < 23) {
                            fprintf(stderr, "Invalid minSdkVersion version, minSdkVersion must be >= 23\n");
                            return false;
                        }
                } else if ((reader.name() == "application"_L1 ||
                            reader.name() == "activity"_L1) &&
                           reader.attributes().hasAttribute("android:label"_L1) &&
                           reader.attributes().value("android:label"_L1) == "@string/app_name"_L1) {
                    checkOldAndroidLabelString = true;
                } else if (reader.name() == "meta-data"_L1) {
                    const auto name = reader.attributes().value("android:name"_L1);
                    const auto value = reader.attributes().value("android:value"_L1);
                    if (name == "android.app.lib_name"_L1 && value.contains(u' ')) {
                        fprintf(stderr, "The Activity's android.app.lib_name should not contain"
                                        " spaces.\n");
                        return false;
                    }
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
    // Use extraLibraryDirs as the extra library lookup folder if it is expected to find a file in
    // any $prefix/lib folder.
    // Library directories from a build tree(extraLibraryDirs) have the higher priority.
    if (relativeFileName.startsWith("lib/"_L1)) {
        for (const auto &dir : options->extraLibraryDirs) {
            const QString path = dir + u'/' + relativeFileName.mid(sizeof("lib/") - 1);
            if (QFile::exists(path))
                return path;
        }
    }

    for (const auto &prefix : options->extraPrefixDirs) {
        const QString path = prefix + u'/' + relativeFileName;
        if (QFile::exists(path))
            return path;
    }

    if (relativeFileName.endsWith("-android-dependencies.xml"_L1)) {
        for (const auto &dir : options->extraLibraryDirs) {
            const QString path = dir + u'/' + relativeFileName;
            if (QFile::exists(path))
                return path;
        }
        return options->qtInstallDirectory + u'/' + options->qtLibsDirectory +
               u'/' + relativeFileName;
    }

    if (relativeFileName.startsWith("jar/"_L1)) {
        return options->qtInstallDirectory + u'/' + options->qtDataDirectory +
               u'/' + relativeFileName;
    }

    if (relativeFileName.startsWith("lib/"_L1)) {
        return options->qtInstallDirectory + u'/' + options->qtLibsDirectory +
               u'/' + relativeFileName.mid(sizeof("lib/") - 1);
    }
    return options->qtInstallDirectory + u'/' + relativeFileName;
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
            ret += findFilesRecursively(options,
                        QFileInfo(info.absoluteFilePath() + QChar(u'/') + entry),
                        rootPath);
        }

        return ret;
    } else {
        return QList<QtDependency>() << QtDependency(info.absoluteFilePath().mid(rootPath.size()), info.absoluteFilePath());
    }
}

QList<QtDependency> findFilesRecursively(const Options &options, const QString &fileName)
{
    // We try to find the fileName in extraPrefixDirs first. The function behaves differently
    // depending on what the fileName points to. If fileName is a file then we try to find the
    // first occurrence in extraPrefixDirs and return this file. If fileName is directory function
    // iterates over it and looks for deployment artifacts in each 'extraPrefixDirs' entry.
    // Also we assume that if the fileName is recognized as a directory once it will be directory
    // for every 'extraPrefixDirs' entry.
    QList<QtDependency> deps;
    for (const auto &prefix : options.extraPrefixDirs) {
        QFileInfo info(prefix + u'/' + fileName);
        if (info.exists()) {
            if (info.isDir())
                deps.append(findFilesRecursively(options, info, prefix + u'/'));
            else
                return findFilesRecursively(options, info, prefix + u'/');
        }
    }

    // Usually android deployment settings contain Qt install directory in extraPrefixDirs.
    if (std::find(options.extraPrefixDirs.begin(), options.extraPrefixDirs.end(),
                  options.qtInstallDirectory) == options.extraPrefixDirs.end()) {
        QFileInfo info(options.qtInstallDirectory + "/"_L1 + fileName);
        QFileInfo rootPath(options.qtInstallDirectory + "/"_L1);
        deps.append(findFilesRecursively(options, info, rootPath.absolutePath()));
    }
    return deps;
}

bool readAndroidDependencyXml(Options *options,
                              const QString &moduleName,
                              QSet<QString> *usedDependencies,
                              QSet<QString> *remainingDependencies)
{
    QString androidDependencyName = absoluteFilePath(options, "%1-android-dependencies.xml"_L1.arg(moduleName));

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
                if (reader.name() == "bundled"_L1) {
                    if (!reader.attributes().hasAttribute("file"_L1)) {
                        fprintf(stderr, "Invalid android dependency file: %s\n", qPrintable(androidDependencyName));
                        return false;
                    }

                    QString file = reader.attributes().value("file"_L1).toString();

                    const QList<QtDependency> fileNames = findFilesRecursively(*options, file);

                    for (const QtDependency &fileName : fileNames) {
                        if (usedDependencies->contains(fileName.absolutePath))
                            continue;

                        if (fileName.absolutePath.endsWith(".so"_L1)) {
                            QSet<QString> remainingDependencies;
                            if (!readDependenciesFromElf(options, fileName.absolutePath,
                                                         usedDependencies,
                                                         &remainingDependencies)) {
                                fprintf(stdout, "Skipping dependencies from xml: %s\n",
                                        qPrintable(fileName.relativePath));
                                continue;
                            }
                        }
                        usedDependencies->insert(fileName.absolutePath);

                        if (options->verbose)
                            fprintf(stdout, "Appending dependency from xml: %s\n", qPrintable(fileName.relativePath));

                        options->qtDependencies[options->currentArchitecture].append(fileName);
                    }
                } else if (reader.name() == "jar"_L1) {
                    int bundling = reader.attributes().value("bundling"_L1).toInt();
                    QString fileName = QDir::cleanPath(reader.attributes().value("file"_L1).toString());
                    if (bundling) {
                        QtDependency dependency(fileName, absoluteFilePath(options, fileName));
                        if (!usedDependencies->contains(dependency.absolutePath)) {
                            options->qtDependencies[options->currentArchitecture].append(dependency);
                            usedDependencies->insert(dependency.absolutePath);
                        }
                    }

                    if (reader.attributes().hasAttribute("initClass"_L1)) {
                        options->initClasses.append(reader.attributes().value("initClass"_L1).toString());
                    }
                } else if (reader.name() == "lib"_L1) {
                    QString fileName = QDir::cleanPath(reader.attributes().value("file"_L1).toString());
                    if (reader.attributes().hasAttribute("replaces"_L1)) {
                        QString replaces = reader.attributes().value("replaces"_L1).toString();
                        for (int i=0; i<options->localLibs.size(); ++i) {
                            if (options->localLibs[options->currentArchitecture].at(i) == replaces) {
                                options->localLibs[options->currentArchitecture][i] = fileName;
                                break;
                            }
                        }
                    } else if (!fileName.isEmpty()) {
                        options->localLibs[options->currentArchitecture].append(fileName);
                    }
                    if (fileName.endsWith(".so"_L1) && checkArchitecture(*options, fileName)) {
                        remainingDependencies->insert(fileName);
                    }
                } else if (reader.name() == "permission"_L1) {
                    QString name = reader.attributes().value("name"_L1).toString();
                    options->permissions.append(name);
                } else if (reader.name() == "feature"_L1) {
                    QString name = reader.attributes().value("name"_L1).toString();
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
    QString readElf = llvmReadobjPath(options);
    if (!QFile::exists(readElf)) {
        fprintf(stderr, "Command does not exist: %s\n", qPrintable(readElf));
        return QStringList();
    }

    readElf = "%1 --needed-libs %2"_L1.arg(shellQuote(readElf), shellQuote(fileName));

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
                auto it = elfArchitectures.find(line.mid(6));
                if (it == elfArchitectures.constEnd() || *it != options.currentArchitecture.toLatin1()) {
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
        QString libraryName = "lib/"_L1 + library;
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

    for (const QString &dependency : std::as_const(dependenciesToCheck)) {
        QString qtBaseName = dependency.mid(sizeof("lib/lib") - 1);
        qtBaseName = qtBaseName.left(qtBaseName.size() - (sizeof(".so") - 1));
        if (!readAndroidDependencyXml(options, qtBaseName, usedDependencies, remainingDependencies)) {
            return false;
        }
    }

    return true;
}

bool scanImports(Options *options, QSet<QString> *usedDependencies)
{
    if (options->verbose)
        fprintf(stdout, "Scanning for QML imports.\n");

    QString qmlImportScanner;
    if (!options->qmlImportScannerBinaryPath.isEmpty()) {
        qmlImportScanner = options->qmlImportScannerBinaryPath;
    } else {
        qmlImportScanner = execSuffixAppended(options->qtLibExecsDirectory +
                                              "/qmlimportscanner"_L1);
    }

    QStringList importPaths;

    // In Conan's case, qtInstallDirectory will point only to qtbase installed files, which
    // lacks a qml directory. We don't want to pass it as an import path if it doesn't exist
    // because it will cause qmlimportscanner to fail.
    // This also covers the case when only qtbase is installed in a regular Qt build.
    const QString mainImportPath = options->qtInstallDirectory + u'/' + options->qtQmlDirectory;
    if (QFile::exists(mainImportPath))
        importPaths += shellQuote(mainImportPath);

    // These are usually provided by CMake in the deployment json file from paths specified
    // in CMAKE_FIND_ROOT_PATH. They might not have qml modules.
    for (const QString &prefix : options->extraPrefixDirs)
        if (QFile::exists(prefix + "/qml"_L1))
            importPaths += shellQuote(prefix + "/qml"_L1);

    // These are provided by both CMake and qmake.
    for (const QString &qmlImportPath : std::as_const(options->qmlImportPaths)) {
        if (QFile::exists(qmlImportPath)) {
            importPaths += shellQuote(qmlImportPath);
        } else {
            fprintf(stderr, "Warning: QML import path %s does not exist.\n",
                    qPrintable(qmlImportPath));
        }
    }

    bool qmlImportExists = false;

    for (const QString &import : importPaths) {
        if (QDir().exists(import)) {
            qmlImportExists = true;
            break;
        }
    }

    // Check importPaths without rootPath, since we need at least one qml plugins
    // folder to run a QML file
    if (!qmlImportExists) {
        fprintf(stderr, "Warning: no 'qml' directory found under Qt install directory "
                "or import paths. Skipping QML dependency scanning.\n");
        return true;
    }

    if (!QFile::exists(qmlImportScanner)) {
        fprintf(stderr, "%s: qmlimportscanner not found at %s\n",
                qmlImportExists ? "Error"_L1.data() : "Warning"_L1.data(),
                qPrintable(qmlImportScanner));
        return true;
    }

    for (auto rootPath : options->rootPaths) {
        rootPath = QFileInfo(rootPath).absoluteFilePath();

        if (!rootPath.endsWith(u'/'))
            rootPath += u'/';

        // After checking for qml folder imports we can add rootPath
        if (!rootPath.isEmpty())
            importPaths += shellQuote(rootPath);

        qmlImportScanner += " -rootPath %1"_L1.arg(shellQuote(rootPath));
    }

    if (!options->qrcFiles.isEmpty()) {
        qmlImportScanner += " -qrcFiles"_L1;
        for (const QString &qrcFile : options->qrcFiles)
            qmlImportScanner += u' ' + shellQuote(qrcFile);
    }

    qmlImportScanner += " -importPath %1"_L1.arg(importPaths.join(u' '));

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
        QString path = object.value("path"_L1).toString();
        if (path.isEmpty()) {
            fprintf(stderr, "Warning: QML import could not be resolved in any of the import paths: %s\n",
                    qPrintable(object.value("name"_L1).toString()));
        } else {
            if (options->verbose)
                fprintf(stdout, "  -- Adding '%s' as QML dependency\n", qPrintable(path));

            QFileInfo info(path);

            // The qmlimportscanner sometimes outputs paths that do not exist.
            if (!info.exists()) {
                if (options->verbose)
                    fprintf(stdout, "    -- Skipping because path does not exist.\n");
                continue;
            }

            QString absolutePath = info.absolutePath();
            if (!absolutePath.endsWith(u'/'))
                absolutePath += u'/';

            const QUrl url(object.value("name"_L1).toString());

            const QString moduleUrlPath = u"/"_s + url.toString().replace(u'.', u'/');
            if (checkCanImportFromRootPaths(options, info.absolutePath(), moduleUrlPath)) {
                if (options->verbose)
                    fprintf(stdout, "    -- Skipping because path is in QML root path.\n");
                continue;
            }

            QString importPathOfThisImport;
            for (const QString &importPath : std::as_const(importPaths)) {
                QString cleanImportPath = QDir::cleanPath(importPath);
                if (QFile::exists(cleanImportPath + moduleUrlPath)) {
                    importPathOfThisImport = importPath;
                    break;
                }
            }

            if (importPathOfThisImport.isEmpty()) {
                fprintf(stderr, "Import found outside of import paths: %s.\n", qPrintable(info.absoluteFilePath()));
                return false;
            }

            importPathOfThisImport = QDir(importPathOfThisImport).absolutePath() + u'/';
            QList<QtDependency> qmlImportsDependencies;
            auto collectQmlDependency = [&usedDependencies, &qmlImportsDependencies,
                                         &importPathOfThisImport](const QString &filePath) {
                if (!usedDependencies->contains(filePath)) {
                    usedDependencies->insert(filePath);
                    qmlImportsDependencies += QtDependency(
                            "qml/"_L1 + filePath.mid(importPathOfThisImport.size()),
                            filePath);
                }
            };

            QString plugin = object.value("plugin"_L1).toString();
            bool pluginIsOptional = object.value("pluginIsOptional"_L1).toBool();
            QFileInfo pluginFileInfo = QFileInfo(
                    path + u'/' + "lib"_L1 + plugin + u'_'
                    + options->currentArchitecture + ".so"_L1);
            QString pluginFilePath = pluginFileInfo.absoluteFilePath();
            QSet<QString> remainingDependencies;
            if (pluginFileInfo.exists() && checkArchitecture(*options, pluginFilePath)
                && readDependenciesFromElf(options, pluginFilePath, usedDependencies,
                                           &remainingDependencies)) {
                collectQmlDependency(pluginFilePath);
            } else if (!pluginIsOptional) {
                if (options->verbose)
                    fprintf(stdout, "    -- Skipping because the required plugin is missing.\n");
                continue;
            }

            QFileInfo qmldirFileInfo = QFileInfo(path + u'/' + "qmldir"_L1);
            if (qmldirFileInfo.exists()) {
                collectQmlDependency(qmldirFileInfo.absoluteFilePath());
            }

            QString prefer = object.value("prefer"_L1).toString();
            // If the preferred location of Qml files points to the Qt resources, this means
            // that all Qml files has been embedded into plugin and we should not copy them to the
            // android rcc bundle
            if (!prefer.startsWith(":/"_L1)) {
                QVariantList qmlFiles =
                        object.value("components"_L1).toArray().toVariantList();
                qmlFiles.append(object.value("scripts"_L1).toArray().toVariantList());
                bool qmlFilesMissing = false;
                for (const auto &qmlFileEntry : qmlFiles) {
                    QFileInfo fileInfo(qmlFileEntry.toString());
                    if (!fileInfo.exists()) {
                        qmlFilesMissing = true;
                        break;
                    }
                    collectQmlDependency(fileInfo.absoluteFilePath());
                }

                if (qmlFilesMissing) {
                    if (options->verbose)
                        fprintf(stdout,
                                "    -- Skipping because the required qml files are missing.\n");
                    continue;
                }
            }

            options->qtDependencies[options->currentArchitecture].append(qmlImportsDependencies);
        }
    }

    return true;
}

bool checkCanImportFromRootPaths(const Options *options, const QString &absolutePath,
                                 const QString &moduleUrlPath)
{
    for (auto rootPath : options->rootPaths) {
        if ((rootPath + moduleUrlPath) == absolutePath)
            return true;
    }
    return false;
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
    auto assetsDir = "%1/assets"_L1.arg(options.outputDirectory);
    if (!QDir{"%1/android_rcc_bundle"_L1.arg(assetsDir)}.exists()) {
        fprintf(stdout, "Skipping createRCC\n");
        return true;
    }

    if (options.verbose)
        fprintf(stdout, "Create rcc bundle.\n");


    QString rcc;
    if (!options.rccBinaryPath.isEmpty()) {
        rcc = options.rccBinaryPath;
    } else {
        rcc = execSuffixAppended(options.qtLibExecsDirectory + "/rcc"_L1);
    }

    if (!QFile::exists(rcc)) {
        fprintf(stderr, "rcc not found: %s\n", qPrintable(rcc));
        return false;
    }
    auto currentDir = QDir::currentPath();
    if (!QDir::setCurrent("%1/android_rcc_bundle"_L1.arg(assetsDir))) {
        fprintf(stderr, "Cannot set current dir to: %s\n", qPrintable("%1/android_rcc_bundle"_L1.arg(assetsDir)));
        return false;
    }

    bool res = runCommand(options, "%1 --project -o %2"_L1.arg(rcc, shellQuote("%1/android_rcc_bundle.qrc"_L1.arg(assetsDir))));
    if (!res)
        return false;

    QLatin1StringView noZstd;
    if (!options.isZstdCompressionEnabled)
        noZstd = "--no-zstd"_L1;

    QFile::rename("%1/android_rcc_bundle.qrc"_L1.arg(assetsDir), "%1/android_rcc_bundle/android_rcc_bundle.qrc"_L1.arg(assetsDir));

    res = runCommand(options, "%1 %2 %3 --binary -o %4 android_rcc_bundle.qrc"_L1.arg(rcc, shellQuote("--root=/android_rcc_bundle/"_L1),
                                                                                      noZstd,
                                                                                      shellQuote("%1/android_rcc_bundle.rcc"_L1.arg(assetsDir))));
    if (!QDir::setCurrent(currentDir)) {
        fprintf(stderr, "Cannot set current dir to: %s\n", qPrintable(currentDir));
        return false;
    }
    if (!options.noRccBundleCleanup) {
        QFile::remove("%1/android_rcc_bundle.qrc"_L1.arg(assetsDir));
        QDir{"%1/android_rcc_bundle"_L1.arg(assetsDir)}.removeRecursively();
    }
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
    if (!readDependenciesFromElf(options, "%1/libs/%2/lib%3_%2.so"_L1.arg(options->outputDirectory, options->currentArchitecture, options->applicationBinary), &usedDependencies, &remainingDependencies))
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
                    qPrintable(unmetDependencies.join(u',')));
        }
    }

    QStringList::iterator it = options->localLibs[options->currentArchitecture].begin();
    while (it != options->localLibs[options->currentArchitecture].end()) {
        QStringList unmetDependencies;
        if (!goodToCopy(options, absoluteFilePath(options, *it), &unmetDependencies)) {
            fprintf(stdout, "Skipping %s due to unmet dependencies: %s\n",
                    qPrintable(*it),
                    qPrintable(unmetDependencies.join(u',')));
            it = options->localLibs[options->currentArchitecture].erase(it);
        } else {
            ++it;
        }
    }

    if (options->qmlSkipImportScanning
        || (options->rootPaths.empty() && options->qrcFiles.isEmpty()))
        return true;
    return scanImports(options, &usedDependencies);
}

bool containsApplicationBinary(Options *options)
{
    if (!options->build)
        return true;

    if (options->verbose)
        fprintf(stdout, "Checking if application binary is in package.\n");

    QString applicationFileName = "lib%1_%2.so"_L1.arg(options->applicationBinary,
                                                       options->currentArchitecture);

    QString applicationPath = "%1/libs/%2/%3"_L1.arg(options->outputDirectory,
                                                     options->currentArchitecture,
                                                     applicationFileName);
    if (!QFile::exists(applicationPath)) {
#if defined(Q_OS_WIN32)
        const auto makeTool = "mingw32-make"_L1; // Only Mingw host builds supported on Windows currently
#else
        const auto makeTool = "make"_L1;
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
    QString adb = execSuffixAppended(options.sdkPath + "/platform-tools/adb"_L1);
    if (!QFile::exists(adb)) {
        fprintf(stderr, "Cannot find adb tool: %s\n", qPrintable(adb));
        return 0;
    }
    QString installOption;
    if (!options.installLocation.isEmpty())
        installOption = " -s "_L1 + shellQuote(options.installLocation);

    adb = "%1%2 %3"_L1.arg(shellQuote(adb), installOption, arguments);

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
    if (!file.endsWith(".so"_L1))
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
            fprintf(stdout, "Copying %zd dependencies from Qt into package.\n", size_t(options->qtDependencies[options->currentArchitecture].size()));
            break;
        case Options::Unbundled:
            fprintf(stdout, "Copying dependencies from Qt into the package build folder,"
                            "skipping native libraries.\n");
            break;
        };
    }

    if (!options->build)
        return true;


    QString libsDirectory = "libs/"_L1;

    // Copy other Qt dependencies
    auto assetsDestinationDirectory = "assets/android_rcc_bundle/"_L1;
    for (const QtDependency &qtDependency : std::as_const(options->qtDependencies[options->currentArchitecture])) {
        QString sourceFileName = qtDependency.absolutePath;
        QString destinationFileName;
        bool isSharedLibrary = qtDependency.relativePath.endsWith(".so"_L1);
        if (isSharedLibrary) {
            QString garbledFileName = qtDependency.relativePath.mid(
                qtDependency.relativePath.lastIndexOf(u'/') + 1);
            destinationFileName = libsDirectory + options->currentArchitecture + u'/' + garbledFileName;
        } else if (QDir::fromNativeSeparators(qtDependency.relativePath).startsWith("jar/"_L1)) {
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
                fprintf(stdout, "  -- Skipping %s. It has unmet dependencies: %s.\n",
                        qPrintable(sourceFileName),
                        qPrintable(unmetDependencies.join(u',')));
            }
            continue;
        }

        if ((isDeployment(options, Options::Bundled) || !isSharedLibrary)
                && !copyFileIfNewer(sourceFileName,
                                    options->outputDirectory + u'/' + destinationFileName,
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

    QFile file(options.outputDirectory + "/project.properties"_L1);
    if (file.open(QIODevice::ReadOnly)) {
        while (!file.atEnd()) {
            QByteArray line = file.readLine().trimmed();
            if (line.startsWith("android.library.reference")) {
                int equalSignIndex = line.indexOf('=');
                if (equalSignIndex >= 0) {
                    QString path = QString::fromLocal8Bit(line.mid(equalSignIndex + 1));

                    QFileInfo info(options.outputDirectory + u'/' + path);
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
        QFileInfo fileInfo(path + u'/' + fileName);
        if (fileInfo.exists() && fileInfo.isFile() && fileInfo.isExecutable())
            return path + u'/' + fileName;
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

        const int idx = line.indexOf('=');
        if (idx > -1)
            properties[line.left(idx).trimmed()] = line.mid(idx + 1).trimmed();
    }
    file.close();
    return properties;
}

static bool mergeGradleProperties(const QString &path, GradleProperties properties)
{
    const QString oldPathStr = path + u'~';
    QFile::remove(oldPathStr);
    QFile::rename(path, oldPathStr);
    QFile file(path);
    if (!file.open(QIODevice::Truncate | QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "Can't open file: %s for writing\n", qPrintable(file.fileName()));
        return false;
    }

    QFile oldFile(oldPathStr);
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
            file.write(line.trimmed() + '\n');
        }
        oldFile.close();
        QFile::remove(oldPathStr);
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
                MAX_PATH, qPrintable(longFileNames.join(u'\n')));
    }
}
#endif

struct GradleFlags {
    bool setsLegacyPackaging = false;
    bool usesIntegerCompileSdkVersion = false;
};

GradleFlags gradleBuildFlags(const QString &path)
{
    GradleFlags flags;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return flags;

    auto isComment = [](const QByteArray &line) {
        const auto trimmed = line.trimmed();
        return trimmed.startsWith("//") || trimmed.startsWith('*') || trimmed.startsWith("/*");
    };

    const auto lines = file.readAll().split('\n');
    for (const auto &line : lines) {
        if (isComment(line))
            continue;
        if (line.contains("useLegacyPackaging")) {
            flags.setsLegacyPackaging = true;
        } else if (line.contains("compileSdkVersion androidCompileSdkVersion.toInteger()")) {
            flags.usesIntegerCompileSdkVersion = true;
        }
    }

    return flags;
}

bool buildAndroidProject(const Options &options)
{
    GradleProperties localProperties;
    localProperties["sdk.dir"] = QDir::fromNativeSeparators(options.sdkPath).toUtf8();
    const QString localPropertiesPath = options.outputDirectory + "local.properties"_L1;
    if (!mergeGradleProperties(localPropertiesPath, localProperties))
        return false;

    const QString gradlePropertiesPath = options.outputDirectory + "gradle.properties"_L1;
    GradleProperties gradleProperties = readGradleProperties(gradlePropertiesPath);

    const QString gradleBuildFilePath = options.outputDirectory + "build.gradle"_L1;
    GradleFlags gradleFlags = gradleBuildFlags(gradleBuildFilePath);
    if (!gradleFlags.setsLegacyPackaging)
        gradleProperties["android.bundle.enableUncompressedNativeLibs"] = "false";

    gradleProperties["buildDir"] = "build";
    gradleProperties["qtAndroidDir"] =
        (options.qtInstallDirectory + u'/' + options.qtDataDirectory +
         "/src/android/java"_L1)
            .toUtf8();
    // The following property "qt5AndroidDir" is only for compatibility.
    // Projects using a custom build.gradle file may use this variable.
    // ### Qt7: Remove the following line
    gradleProperties["qt5AndroidDir"] =
        (options.qtInstallDirectory + u'/' + options.qtDataDirectory +
         "/src/android/java"_L1)
            .toUtf8();

    QByteArray sdkPlatformVersion;
    // Provide the integer version only if build.gradle explicitly converts to Integer,
    // to avoid regression to existing projects that build for sdk platform of form android-xx.
    if (gradleFlags.usesIntegerCompileSdkVersion) {
        const QByteArray tmp = options.androidPlatform.split(u'-').last().toLocal8Bit();
        bool ok;
        tmp.toInt(&ok);
        if (ok) {
            sdkPlatformVersion = tmp;
        } else {
            fprintf(stderr, "Warning: Gradle expects SDK platform version to be an integer, "
                            "but the set version is not convertible to an integer.");
        }
    }

    if (sdkPlatformVersion.isEmpty())
        sdkPlatformVersion = options.androidPlatform.toLocal8Bit();

    gradleProperties["androidCompileSdkVersion"] = sdkPlatformVersion;
    gradleProperties["qtMinSdkVersion"] = options.minSdkVersion;
    gradleProperties["qtTargetSdkVersion"] = options.targetSdkVersion;
    gradleProperties["androidNdkVersion"] = options.ndkVersion.toUtf8();
    if (gradleProperties["androidBuildToolsVersion"].isEmpty())
        gradleProperties["androidBuildToolsVersion"] = options.sdkBuildToolsVersion.toLocal8Bit();
    QString abiList;
    for (auto it = options.architectures.constBegin(); it != options.architectures.constEnd(); ++it) {
        if (!it->enabled)
            continue;
        if (abiList.size())
            abiList.append(u",");
        abiList.append(it.key());
    }
    gradleProperties["qtTargetAbiList"] = abiList.toLocal8Bit();// armeabi-v7a or arm64-v8a or ...
    if (!mergeGradleProperties(gradlePropertiesPath, gradleProperties))
        return false;

    QString gradlePath = batSuffixAppended(options.outputDirectory + "gradlew"_L1);
#ifndef Q_OS_WIN32
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

    QString commandLine = "%1 %2"_L1.arg(shellQuote(gradlePath), options.releasePackage ? " assembleRelease"_L1 : " assembleDebug"_L1);
    if (options.buildAAB)
        commandLine += " bundle"_L1;

    if (options.verbose)
        commandLine += " --info"_L1;

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


    FILE *adbCommand = runAdb(options, " uninstall "_L1 + shellQuote(options.packageName));
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
    path += "/build/outputs/%1/"_L1.arg(pt >= UnsignedAPK ? QStringLiteral("apk") : QStringLiteral("bundle"));
    QString buildType(options.releasePackage ? "release/"_L1 : "debug/"_L1);
    if (QDir(path + buildType).exists())
        path += buildType;
    path += QDir(options.outputDirectory).dirName() + u'-';
    if (options.releasePackage) {
        path += "release-"_L1;
        if (pt >= UnsignedAPK) {
            if (pt == UnsignedAPK)
                path += "un"_L1;
            path += "signed.apk"_L1;
        } else {
            path.chop(1);
            path += ".aab"_L1;
        }
    } else {
        path += "debug"_L1;
        if (pt >= UnsignedAPK) {
            if (pt == SignedAPK)
                path += "-signed"_L1;
            path += ".apk"_L1;
        } else {
            path += ".aab"_L1;
        }
    }
    return path;
}

bool installApk(const Options &options)
{
    fflush(stdout);
    // Uninstall if necessary
    if (options.uninstallApk)
        uninstallApk(options);

    if (options.verbose)
        fprintf(stdout, "Installing Android package to device.\n");

    FILE *adbCommand = runAdb(options, " install -r "_L1
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
    if (isDeployment(options, Options::Unbundled))
        return true;
    if (options->verbose)
        fprintf(stdout, "Copying STL library\n");

    const QString triple = options->architectures[options->currentArchitecture].triple;
    const QString stdCppPath = "%1/%2/lib%3.so"_L1.arg(options->stdCppPath, triple,
                                                       options->stdCppName);
    if (!QFile::exists(stdCppPath)) {
        fprintf(stderr, "STL library does not exist at %s\n", qPrintable(stdCppPath));
        fflush(stdout);
        fflush(stderr);
        return false;
    }

    const QString destinationFile = "%1/libs/%2/lib%3.so"_L1.arg(options->outputDirectory,
                                                                 options->currentArchitecture,
                                                                 options->stdCppName);
    return copyFileIfNewer(stdCppPath, destinationFile, *options);
}

static QString zipalignPath(const Options &options, bool *ok)
{
    *ok = true;
    QString zipAlignTool = execSuffixAppended(options.sdkPath + "/tools/zipalign"_L1);
    if (!QFile::exists(zipAlignTool)) {
        zipAlignTool = execSuffixAppended(options.sdkPath + "/build-tools/"_L1 +
                                          options.sdkBuildToolsVersion + "/zipalign"_L1);
        if (!QFile::exists(zipAlignTool)) {
            fprintf(stderr, "zipalign tool not found: %s\n", qPrintable(zipAlignTool));
            *ok = false;
        }
    }

    return zipAlignTool;
}

bool signAAB(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Signing Android package.\n");

    QString jdkPath = options.jdkPath;

    if (jdkPath.isEmpty())
        jdkPath = QString::fromLocal8Bit(qgetenv("JAVA_HOME"));

    QString jarSignerTool = execSuffixAppended("jarsigner"_L1);
    if (jdkPath.isEmpty() || !QFile::exists(jdkPath + "/bin/"_L1 + jarSignerTool))
        jarSignerTool = findInPath(jarSignerTool);
    else
        jarSignerTool = jdkPath + "/bin/"_L1 + jarSignerTool;

    if (!QFile::exists(jarSignerTool)) {
        fprintf(stderr, "Cannot find jarsigner in JAVA_HOME or PATH. Please use --jdk option to pass in the correct path to JDK.\n");
        return false;
    }

    jarSignerTool = "%1 -sigalg %2 -digestalg %3 -keystore %4"_L1
            .arg(shellQuote(jarSignerTool), shellQuote(options.sigAlg), shellQuote(options.digestAlg), shellQuote(options.keyStore));

    if (!options.keyStorePassword.isEmpty())
        jarSignerTool += " -storepass %1"_L1.arg(shellQuote(options.keyStorePassword));

    if (!options.storeType.isEmpty())
        jarSignerTool += " -storetype %1"_L1.arg(shellQuote(options.storeType));

    if (!options.keyPass.isEmpty())
        jarSignerTool += " -keypass %1"_L1.arg(shellQuote(options.keyPass));

    if (!options.sigFile.isEmpty())
        jarSignerTool += " -sigfile %1"_L1.arg(shellQuote(options.sigFile));

    if (!options.signedJar.isEmpty())
        jarSignerTool += " -signedjar %1"_L1.arg(shellQuote(options.signedJar));

    if (!options.tsaUrl.isEmpty())
        jarSignerTool += " -tsa %1"_L1.arg(shellQuote(options.tsaUrl));

    if (!options.tsaCert.isEmpty())
        jarSignerTool += " -tsacert %1"_L1.arg(shellQuote(options.tsaCert));

    if (options.internalSf)
        jarSignerTool += " -internalsf"_L1;

    if (options.sectionsOnly)
        jarSignerTool += " -sectionsonly"_L1;

    if (options.protectedAuthenticationPath)
        jarSignerTool += " -protected"_L1;

    auto jarSignPackage = [&](const QString &file) {
        fprintf(stdout, "Signing file %s\n", qPrintable(file));
        fflush(stdout);
        QString command = jarSignerTool + " %1 %2"_L1.arg(shellQuote(file))
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

    if (options.buildAAB && !jarSignPackage(packagePath(options, AAB)))
        return false;
    return true;
}

bool signPackage(const Options &options)
{
    const QString apksignerTool = batSuffixAppended(options.sdkPath + "/build-tools/"_L1 +
                                              options.sdkBuildToolsVersion + "/apksigner"_L1);
    // APKs signed with apksigner must not be changed after they're signed,
    // therefore we need to zipalign it before we sign it.

    bool ok;
    QString zipAlignTool = zipalignPath(options, &ok);
    if (!ok)
        return false;

    auto zipalignRunner = [](const QString &zipAlignCommandLine) {
        FILE *zipAlignCommand = openProcess(zipAlignCommandLine);
        if (zipAlignCommand == 0) {
            fprintf(stderr, "Couldn't run zipalign.\n");
            return false;
        }

        char buffer[512];
        while (fgets(buffer, sizeof(buffer), zipAlignCommand) != 0)
            fprintf(stdout, "%s", buffer);

        return pclose(zipAlignCommand) == 0;
    };

    const QString verifyZipAlignCommandLine =
            "%1%2 -c 4 %3"_L1
                    .arg(shellQuote(zipAlignTool),
                         options.verbose ? " -v"_L1 : QLatin1StringView(),
                         shellQuote(packagePath(options, UnsignedAPK)));

    if (zipalignRunner(verifyZipAlignCommandLine)) {
        if (options.verbose)
            fprintf(stdout, "APK already aligned, copying it for signing.\n");

        if (QFile::exists(packagePath(options, SignedAPK)))
            QFile::remove(packagePath(options, SignedAPK));

        if (!QFile::copy(packagePath(options, UnsignedAPK), packagePath(options, SignedAPK))) {
            fprintf(stderr, "Could not copy unsigned APK.\n");
            return false;
        }
    } else {
        if (options.verbose)
            fprintf(stdout, "APK not aligned, aligning it for signing.\n");

        const QString zipAlignCommandLine =
                "%1%2 -f 4 %3 %4"_L1
                        .arg(shellQuote(zipAlignTool),
                             options.verbose ? " -v"_L1 : QLatin1StringView(),
                             shellQuote(packagePath(options, UnsignedAPK)),
                             shellQuote(packagePath(options, SignedAPK)));

        if (!zipalignRunner(zipAlignCommandLine)) {
            fprintf(stderr, "zipalign command failed.\n");
            if (!options.verbose)
                fprintf(stderr, "  -- Run with --verbose for more information.\n");
            return false;
        }
    }

    QString apkSignCommand = "%1 sign --ks %2"_L1
            .arg(shellQuote(apksignerTool), shellQuote(options.keyStore));

    if (!options.keyStorePassword.isEmpty())
        apkSignCommand += " --ks-pass pass:%1"_L1.arg(shellQuote(options.keyStorePassword));

    if (!options.keyStoreAlias.isEmpty())
        apkSignCommand += " --ks-key-alias %1"_L1.arg(shellQuote(options.keyStoreAlias));

    if (!options.keyPass.isEmpty())
        apkSignCommand += " --key-pass pass:%1"_L1.arg(shellQuote(options.keyPass));

    if (options.verbose)
        apkSignCommand += " --verbose"_L1;

    apkSignCommand += " %1"_L1.arg(shellQuote(packagePath(options, SignedAPK)));

    auto apkSignerRunner = [](const QString &command, bool verbose) {
        FILE *apkSigner = openProcess(command);
        if (apkSigner == 0) {
            fprintf(stderr, "Couldn't run apksigner.\n");
            return false;
        }

        char buffer[512];
        while (fgets(buffer, sizeof(buffer), apkSigner) != 0)
            fprintf(stdout, "%s", buffer);

        int errorCode = pclose(apkSigner);
        if (errorCode != 0) {
            fprintf(stderr, "apksigner command failed.\n");
            if (!verbose)
                fprintf(stderr, "  -- Run with --verbose for more information.\n");
            return false;
        }
        return true;
    };

    // Sign the package
    if (!apkSignerRunner(apkSignCommand, options.verbose))
        return false;

    const QString apkVerifyCommand =
            "%1 verify --verbose %2"_L1
                    .arg(shellQuote(apksignerTool), shellQuote(packagePath(options, SignedAPK)));

    if (options.buildAAB && !signAAB(options))
      return false;

    // Verify the package and remove the unsigned apk
    return apkSignerRunner(apkVerifyCommand, true) && QFile::remove(packagePath(options, UnsignedAPK));
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
    CannotCreateAndroidProject = 13, // Not used anymore
    CannotBuildAndroidProject = 14,
    CannotSignPackage = 15,
    CannotInstallApk = 16,
    CannotCopyAndroidExtraResources = 19,
    CannotCopyApk = 20,
    CannotCreateRcc = 21
};

bool writeDependencyFile(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Writing dependency file.\n");

    QString relativeTargetPath;
    if (options.copyDependenciesOnly) {
        // When androiddeploy Qt is running in copyDependenciesOnly mode we need to use
        // the timestamp file as the target to collect dependencies.
        QString timestampAbsPath = QFileInfo(options.depFilePath).absolutePath() + "/timestamp"_L1;
        relativeTargetPath = QDir(options.buildDirectory).relativeFilePath(timestampAbsPath);
    } else {
        relativeTargetPath = QDir(options.buildDirectory).relativeFilePath(options.apkPath);
    }

    QFile depFile(options.depFilePath);
    if (depFile.open(QIODevice::WriteOnly)) {
        depFile.write(escapeAndEncodeDependencyPath(relativeTargetPath));
        depFile.write(": ");

        for (const auto &file : dependenciesForDepfile) {
            depFile.write(" \\\n    ");
            depFile.write(escapeAndEncodeDependencyPath(file));
        }

        depFile.write("\n");
    }
    return true;
}

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
        fprintf(stdout, "[TIMING] %lld ns: Read input file\n", options.timer.nsecsElapsed());

    fprintf(stdout,
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

    bool androidTemplatetCopied = false;

    for (auto it = options.architectures.constBegin(); it != options.architectures.constEnd(); ++it) {
        if (!it->enabled)
            continue;
        options.setCurrentQtArchitecture(it.key(),
                                         it.value().qtInstallDirectory,
                                         it.value().qtDirectories);

        // All architectures have a copy of the gradle files but only one set needs to be copied.
        if (!androidTemplatetCopied && options.build && !options.auxMode && !options.copyDependenciesOnly) {
            cleanAndroidFiles(options);
            if (Q_UNLIKELY(options.timing))
                fprintf(stdout, "[TIMING] %lld ns: Cleaned Android file\n", options.timer.nsecsElapsed());

            if (!copyAndroidTemplate(options))
                return CannotCopyAndroidTemplate;

            if (Q_UNLIKELY(options.timing))
                fprintf(stdout, "[TIMING] %lld ns: Copied Android template\n", options.timer.nsecsElapsed());
            androidTemplatetCopied = true;
        }

        if (!readDependencies(&options))
            return CannotReadDependencies;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Read dependencies\n", options.timer.nsecsElapsed());

        if (!copyQtFiles(&options))
            return CannotCopyQtFiles;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Copied Qt files\n", options.timer.nsecsElapsed());

        if (!copyAndroidExtraLibs(&options))
            return CannotCopyAndroidExtraLibs;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ms: Copied extra libs\n", options.timer.nsecsElapsed());

        if (!copyAndroidExtraResources(&options))
            return CannotCopyAndroidExtraResources;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Copied extra resources\n", options.timer.nsecsElapsed());

        if (!options.auxMode) {
            if (!copyStdCpp(&options))
                return CannotCopyGnuStl;

            if (Q_UNLIKELY(options.timing))
                fprintf(stdout, "[TIMING] %lld ns: Copied GNU STL\n", options.timer.nsecsElapsed());
        }
        // If Unbundled deployment is used, remove app lib as we don't want it packaged inside the APK
        if (options.deploymentMechanism == Options::Unbundled) {
            QString appLibPath = "%1/libs/%2/lib%3_%2.so"_L1.
                    arg(options.outputDirectory,
                        options.currentArchitecture,
                        options.applicationBinary);
            QFile::remove(appLibPath);
        } else if (!containsApplicationBinary(&options)) {
            return CannotFindApplicationBinary;
        }

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Checked for application binary\n", options.timer.nsecsElapsed());

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Bundled Qt libs\n", options.timer.nsecsElapsed());
    }

    if (options.copyDependenciesOnly) {
        if (!options.depFilePath.isEmpty())
            writeDependencyFile(options);
        return 0;
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
            fprintf(stdout, "[TIMING] %lld ns: Copied android sources\n", options.timer.nsecsElapsed());

        if (!updateAndroidFiles(options))
            return CannotUpdateAndroidFiles;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Updated files\n", options.timer.nsecsElapsed());

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Created project\n", options.timer.nsecsElapsed());

        if (!buildAndroidProject(options))
            return CannotBuildAndroidProject;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Built project\n", options.timer.nsecsElapsed());

        if (!options.keyStore.isEmpty() && !signPackage(options))
            return CannotSignPackage;

        if (!options.apkPath.isEmpty() && !copyPackage(options))
            return CannotCopyApk;

        if (Q_UNLIKELY(options.timing))
            fprintf(stdout, "[TIMING] %lld ns: Signed package\n", options.timer.nsecsElapsed());
    }

    if (options.installApk && !installApk(options))
        return CannotInstallApk;

    if (Q_UNLIKELY(options.timing))
        fprintf(stdout, "[TIMING] %lld ns: Installed APK\n", options.timer.nsecsElapsed());

    if (!options.depFilePath.isEmpty())
        writeDependencyFile(options);

    fprintf(stdout, "Android package built successfully in %.3f ms.\n", options.timer.elapsed() / 1000.);

    if (options.installApk)
        fprintf(stdout, "  -- It can now be run from the selected device/emulator.\n");

    fprintf(stdout, "  -- File: %s\n", qPrintable(packagePath(options, options.keyStore.isEmpty() ? UnsignedAPK
                                                                                              : SignedAPK)));
    fflush(stdout);
    return 0;
}
