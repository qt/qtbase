// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QSet>
#include <QElapsedTimer>
#include <QRegularExpression>

#include "../shared/depfile_shared.h"

using namespace Qt::StringLiterals;

// Global list of all source files that contribute to the HAP
// Used to generate dependency file for CMake DEPFILE support
static QStringList dependenciesForDepfile;

struct Options
{
    QString inputFile;
    QString outputDirectory;
    QString hvigorPath;
    QString applicationBinary;
    QStringList projectLibraries;  // Project-built libraries from CMake
    QString harmonyOsPackageSourceDirectory;
    QString harmonyOsAppName;
    QString harmonyOsAppBundleName;
    QString sdkRoot;
    QString ndkRoot;
    QStringList qmlRootPaths;
    QStringList qmlImportPaths;
    QStringList pluginsImportPaths; // Build-tree plugin search paths (processed before qtPluginsDirectory)
    QStringList targetArchs;

    // Qt installation directories (following androiddeployqt pattern)
    QString qtLibsDirectory;     // Target Qt libs
    QString qtPluginsDirectory;  // Target Qt plugins
    QString qtQmlDirectory;      // Target Qt QML modules
    QString qtLibExecsDirectory; // Host Qt tools (qmlimportscanner, etc.)
    QString qtHostDirectory;     // Host Qt installation
    QStringList extraLibsDirs;   // Extra library search paths (e.g. HARMONYOS_DEPS_ROOT/lib)

    bool verbose = false;
    bool releaseMode = false;
    bool installApk = false;     // Keep name for consistency with androiddeployqt
    bool buildPackage = true;

    QString depFilePath;     // Path to write dependency file
    QString depFileBase;     // Base directory for relative paths in depfile

    // HarmonyOS permissions injected via qt_add_harmonyos_permission
    QJsonArray permissions;

    // App-level metadata from the QT_HARMONYOS_APP_* target properties.
    // Empty/zero means "user did not set this", so harmonydeployqt leaves the
    // template default in place.
    QString harmonyOsAppVendor;
    int harmonyOsAppVersionCode = 0;
    QString harmonyOsAppVersionName;
    QString harmonyOsAppLabel;
    QString harmonyOsAppIcon;

    // SDK versions from the QT_HARMONYOS_*_SDK_VERSION target properties.
    // Substituted into entry/build-profile.json5. Empty means "leave template
    // default".
    QString harmonyOsCompatibleSdkVersion;
    QString harmonyOsTargetSdkVersion;
    QString harmonyOsCompileSdkVersion;

    // Additional plugin .so files from QT_HARMONYOS_EXTRA_PLUGINS.
    QStringList extraPlugins;

    // Module-level metadata from the QT_HARMONYOS_MODULE_* and
    // QT_HARMONYOS_ABILITY_* target properties.
    QString harmonyOsModuleDescription;
    QStringList harmonyOsModuleDeviceTypes;
    QString harmonyOsAbilityOrientation;

    // Test bundle mode
    bool testBundleMode = false;
    QString testBinariesDirectory;   // Directory to scan for libtst_*.so
    QStringList testExcludeList;     // Filenames to exclude from test bundle

    // HAP signing material from --signing-* CLI flags (empty = use env vars).
    QString signingCertPath;
    QString signingProfile;
    QString signingStoreFile;
    QString signingKeyAlias;
    QString signingKeyPassword;
    QString signingStorePassword;
    QString signingAlg;

    QElapsedTimer timer;
};

static void printHelp()
{
    fprintf(stdout, "Usage: harmonydeployqt [options]\n\n"
                    "Options:\n"
                    "  --input <file>              JSON configuration file (required)\n"
                    "  --output <dir>              Output directory for generated project\n"
                    "  --hvigor <path>             Path to hvigorw for building HAP (or set QT_HARMONYOS_HVIGOR)\n"
                    "  --install                   Install HAP to connected device via hdc\n"
                    "  --release                   Build release configuration (default: debug)\n"
                    "  --verbose                   Enable verbose output\n"
                    "  --no-build                  Skip building the HAP\n"
                    "  --test-bundle               Enable test bundle mode (bundles all test binaries into one HAP)\n"
                    "  --depfile <path>            Write dependency file for build system\n"
                    "  --depfile-base <dir>        Base directory for relative paths in depfile\n"
                    "  --signing-cert-path <p>     .cer file (or QT_HARMONYOS_SIGNING_CERT_PATH)\n"
                    "  --signing-profile <p>       .p7b profile (or QT_HARMONYOS_SIGNING_PROFILE)\n"
                    "  --signing-store-file <p>    .p12 keystore (or QT_HARMONYOS_SIGNING_STORE_FILE)\n"
                    "  --signing-key-alias <a>     Key alias (or QT_HARMONYOS_SIGNING_KEY_ALIAS)\n"
                    "  --signing-key-password <s>  Encrypted key pwd (or QT_HARMONYOS_SIGNING_KEY_PASSWORD)\n"
                    "  --signing-store-password <s> Encrypted store pwd (or QT_HARMONYOS_SIGNING_STORE_PASSWORD)\n"
                    "  --signing-alg <alg>         Signature algorithm, default SHA256withECDSA\n"
                    "                              (or QT_HARMONYOS_SIGNING_ALG)\n"
                    "  --help                      Show this help\n\n"
                    "Signing: CLI flags above win per field over the matching env vars.\n"
                    "Passwords must be hvigor-encrypted blobs. If any signing input is set,\n"
                    "all six required values must be present, or the HAP is left unsigned.\n");
}

class QProcessExt : public QProcess
{
public:
    QProcessExt() {
        connect(this, &QProcess::readyReadStandardOutput, [this]() {
            QByteArray output = readAllStandardOutput();
            QString text = QString::fromUtf8(output);
            fprintf(stderr, "harmonydeployqt: external application output: %s\n", qPrintable(text));
        });
        connect(this, &QProcess::readyReadStandardError, [this]() {
            QByteArray error = readAllStandardError();
            QString text = QString::fromUtf8(error);
            fprintf(stderr, "harmonydeployqt: external application error: %s\n", qPrintable(text));
        });
    }
};

static bool parseCommandLine(const QStringList &arguments, Options *options)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Qt HarmonyOS Deployment Tool"_L1);

    QCommandLineOption inputOption("input"_L1, "JSON configuration file"_L1, "file"_L1);
    QCommandLineOption outputOption("output"_L1, "Output directory"_L1, "dir"_L1);
    QCommandLineOption hvigorOption("hvigor"_L1, "Path to hvigorw"_L1, "path"_L1);
    QCommandLineOption installOption("install"_L1, "Install to device"_L1);
    QCommandLineOption releaseOption("release"_L1, "Build release configuration"_L1);
    QCommandLineOption verboseOption("verbose"_L1, "Verbose output"_L1);
    QCommandLineOption noBuildOption("no-build"_L1, "Skip building"_L1);
    QCommandLineOption testBundleOption("test-bundle"_L1, "Enable test bundle mode"_L1);
    QCommandLineOption depfileOption("depfile"_L1, "Dependency file output"_L1, "path"_L1);
    QCommandLineOption depfileBaseOption("depfile-base"_L1, "Base directory for depfile paths"_L1, "dir"_L1);
    QCommandLineOption signingCertPathOption("signing-cert-path"_L1,
        "Path to the .cer file"_L1, "path"_L1);
    QCommandLineOption signingProfileOption("signing-profile"_L1,
        "Path to the .p7b profile"_L1, "path"_L1);
    QCommandLineOption signingStoreFileOption("signing-store-file"_L1,
        "Path to the .p12 keystore"_L1, "path"_L1);
    QCommandLineOption signingKeyAliasOption("signing-key-alias"_L1,
        "Key alias inside the keystore"_L1, "alias"_L1);
    QCommandLineOption signingKeyPasswordOption("signing-key-password"_L1,
        "Encrypted key password"_L1, "pwd"_L1);
    QCommandLineOption signingStorePasswordOption("signing-store-password"_L1,
        "Encrypted keystore password"_L1, "pwd"_L1);
    QCommandLineOption signingAlgOption("signing-alg"_L1,
        "Signature algorithm (default SHA256withECDSA)"_L1, "alg"_L1);
    QCommandLineOption helpOption("help"_L1, "Show help"_L1);

    parser.addOption(inputOption);
    parser.addOption(outputOption);
    parser.addOption(hvigorOption);
    parser.addOption(installOption);
    parser.addOption(releaseOption);
    parser.addOption(verboseOption);
    parser.addOption(noBuildOption);
    parser.addOption(testBundleOption);
    parser.addOption(depfileOption);
    parser.addOption(depfileBaseOption);
    parser.addOption(signingCertPathOption);
    parser.addOption(signingProfileOption);
    parser.addOption(signingStoreFileOption);
    parser.addOption(signingKeyAliasOption);
    parser.addOption(signingKeyPasswordOption);
    parser.addOption(signingStorePasswordOption);
    parser.addOption(signingAlgOption);
    parser.addOption(helpOption);

    if (!parser.parse(arguments)) {
        fprintf(stderr, "%s\n", qPrintable(parser.errorText()));
        return false;
    }

    if (parser.isSet(helpOption)) {
        printHelp();
        return false;
    }

    if (!parser.isSet(inputOption)) {
        fprintf(stderr, "Error: --input option is required\n");
        printHelp();
        return false;
    }

    options->inputFile = parser.value(inputOption);
    options->outputDirectory = parser.value(outputOption);
    options->hvigorPath = parser.value(hvigorOption);
    options->installApk = parser.isSet(installOption);
    options->releaseMode = parser.isSet(releaseOption);
    options->verbose = parser.isSet(verboseOption);
    options->buildPackage = !parser.isSet(noBuildOption);
    options->testBundleMode = parser.isSet(testBundleOption);
    options->depFilePath = parser.value(depfileOption);
    options->depFileBase = parser.value(depfileBaseOption);
    options->signingCertPath = parser.value(signingCertPathOption);
    options->signingProfile = parser.value(signingProfileOption);
    options->signingStoreFile = parser.value(signingStoreFileOption);
    options->signingKeyAlias = parser.value(signingKeyAliasOption);
    options->signingKeyPassword = parser.value(signingKeyPasswordOption);
    options->signingStorePassword = parser.value(signingStorePasswordOption);
    options->signingAlg = parser.value(signingAlgOption);

    return true;
}

static bool readInputConfiguration(Options *options)
{
    QFile inputFile(options->inputFile);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "Failed to open input file: %s\n", qPrintable(options->inputFile));
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(inputFile.readAll(), &parseError);
    if (doc.isNull()) {
        fprintf(stderr, "Failed to parse JSON: %s\n", qPrintable(parseError.errorString()));
        return false;
    }

    QJsonObject obj = doc.object();

    options->applicationBinary = obj["application-binary"_L1].toString();
    options->harmonyOsPackageSourceDirectory = obj["harmonyos-package-source-directory"_L1].toString();
    options->harmonyOsAppName = obj["harmonyos-app-name"_L1].toString();
    options->harmonyOsAppBundleName = obj["harmonyos-app-bundle-name"_L1].toString();
    options->sdkRoot = obj["sdk-root"_L1].toString();
    options->ndkRoot = obj["ndk-root"_L1].toString();
    // qml-root-path may be a string (legacy) or an array (current).
    {
        const QJsonValue rootPathValue = obj["qml-root-path"_L1];
        if (rootPathValue.isArray()) {
            for (const QJsonValue &v : rootPathValue.toArray()) {
                const QString s = v.toString();
                if (!s.isEmpty())
                    options->qmlRootPaths.append(s);
            }
        } else {
            const QString s = rootPathValue.toString();
            if (!s.isEmpty())
                options->qmlRootPaths.append(s);
        }
    }

    // Qt installation directories
    options->qtLibsDirectory = obj["qtLibsDirectory"_L1].toString();
    options->qtPluginsDirectory = obj["qtPluginsDirectory"_L1].toString();
    options->qtQmlDirectory = obj["qtQmlDirectory"_L1].toString();
    options->qtLibExecsDirectory = obj["qtLibExecsDirectory"_L1].toString();
    options->qtHostDirectory = obj["qtHostDirectory"_L1].toString();

    QJsonArray extraLibsDirsArray = obj["extra-libs-dirs"_L1].toArray();
    for (const QJsonValue &value : extraLibsDirsArray)
        options->extraLibsDirs.append(value.toString());

    // Test bundle mode settings (JSON can override CLI flag)
    if (obj["test-bundle"_L1].toBool())
        options->testBundleMode = true;
    options->testBinariesDirectory = obj["test-binaries-directory"_L1].toString();
    QJsonArray excludeArray = obj["test-exclude-list"_L1].toArray();
    for (const QJsonValue &value : excludeArray)
        options->testExcludeList.append(value.toString());

    // Parse project libraries
    QJsonArray projectLibsArray = obj["project-libraries"_L1].toArray();
    for (const QJsonValue &value : projectLibsArray)
        options->projectLibraries.append(value.toString());

    // Parse QML import paths
    QJsonArray importPathsArray = obj["qml-import-paths"_L1].toArray();
    for (const QJsonValue &value : importPathsArray)
        options->qmlImportPaths.append(value.toString());

    // Parse plugins import paths
    QJsonArray pluginsImportPathsArray = obj["plugins-import-paths"_L1].toArray();
    for (const QJsonValue &value : pluginsImportPathsArray)
        options->pluginsImportPaths.append(value.toString());

    // Parse target architectures
    QJsonArray archArray = obj["harmonyos-target-arch"_L1].toArray();
    for (const QJsonValue &value : archArray)
        options->targetArchs.append(value.toString());
    if (options->targetArchs.isEmpty())
        options->targetArchs.append("arm64-v8a"_L1);

    // Parse HarmonyOS permissions injected via qt_add_harmonyos_permission
    options->permissions = obj["permissions"_L1].toArray();

    // App-level metadata injected via the QT_HARMONYOS_APP_* target properties.
    options->harmonyOsAppVendor = obj["harmonyos-app-vendor"_L1].toString();
    options->harmonyOsAppVersionCode = obj["harmonyos-app-version-code"_L1].toInt();
    options->harmonyOsAppVersionName = obj["harmonyos-app-version-name"_L1].toString();
    options->harmonyOsAppLabel = obj["harmonyos-app-label"_L1].toString();
    options->harmonyOsAppIcon = obj["harmonyos-app-icon"_L1].toString();

    // SDK versions for entry/build-profile.json5.
    options->harmonyOsCompatibleSdkVersion =
        obj["harmonyos-compatible-sdk-version"_L1].toString();
    options->harmonyOsTargetSdkVersion =
        obj["harmonyos-target-sdk-version"_L1].toString();
    options->harmonyOsCompileSdkVersion =
        obj["harmonyos-compile-sdk-version"_L1].toString();

    // Extra plugins (resolved file paths). Categories are derived from the
    // parent directory name of each path.
    {
        const QJsonArray extraPluginsArray =
            obj["harmonyos-extra-plugins"_L1].toArray();
        for (const QJsonValue &v : extraPluginsArray) {
            const QString s = v.toString();
            if (!s.isEmpty())
                options->extraPlugins.append(s);
        }
    }

    // Module-level metadata injected via the QT_HARMONYOS_MODULE_* and
    // QT_HARMONYOS_ABILITY_* target properties.
    options->harmonyOsModuleDescription = obj["harmonyos-module-description"_L1].toString();
    const QJsonArray deviceTypesArray = obj["harmonyos-module-device-types"_L1].toArray();
    for (const QJsonValue &value : deviceTypesArray)
        options->harmonyOsModuleDeviceTypes.append(value.toString());
    options->harmonyOsAbilityOrientation = obj["harmonyos-ability-orientation"_L1].toString();

    // Validate required fields
    if (!options->testBundleMode) {
        if (options->applicationBinary.isEmpty()) {
            fprintf(stderr, "Error: 'application-binary' not specified in JSON\n");
            return false;
        }

        // The settings file is generated at CMake generate time, so it names the
        // application binary long before the build produces it. Fail early with a
        // clear reason instead of copying the whole template first and then
        // tripping over the missing file in copyApplicationBinary().
        if (!QFile::exists(options->applicationBinary)) {
            fprintf(stderr, "Error: application binary does not exist: %s\n",
                    qPrintable(options->applicationBinary));
            fprintf(stderr, "       Build the project before running harmonydeployqt.\n");
            return false;
        }
    }

    // Set defaults for test bundle mode
    if (options->testBundleMode) {
        if (options->harmonyOsAppBundleName.isEmpty())
            options->harmonyOsAppBundleName = "org.qtproject.autotests"_L1;
        if (options->harmonyOsAppName.isEmpty())
            options->harmonyOsAppName = "QtAutoTests"_L1;
    }

    // Auto-detect template directory if not specified
    if (options->harmonyOsPackageSourceDirectory.isEmpty()) {
        // For test bundle mode, use qtLibsDirectory as starting point;
        // otherwise start from the application binary location
        QString searchPath;
        if (options->testBundleMode && !options->qtLibsDirectory.isEmpty()) {
            searchPath = QDir::cleanPath(options->qtLibsDirectory);
        } else if (!options->applicationBinary.isEmpty()) {
            QFileInfo appInfo(options->applicationBinary);
            searchPath = QDir::cleanPath(appInfo.absolutePath());
        }

        if (searchPath.isEmpty()) {
            fprintf(stderr, "Error: 'harmonyos-package-source-directory' not specified in JSON\n");
            fprintf(stderr, "       and could not auto-detect template location (no search path)\n");
            return false;
        }

        if (options->verbose)
            fprintf(stdout, "Searching for template starting from: %s\n", qPrintable(searchPath));

        // Walk up directory tree to find Qt installation using string manipulation
        for (int i = 0; i < 10; ++i) {
            // Check for installed template in share directory (matches CMakeLists.txt install path)
            QString templatePath = searchPath + "/share/qt6/src/harmonyos/templates"_L1;
            if (options->verbose) {
                fprintf(stdout, "  Checking: %s ... %s\n", qPrintable(templatePath),
                       QDir(templatePath).exists() ? "FOUND" : "not found");
            }
            if (QDir(templatePath).exists()) {
                options->harmonyOsPackageSourceDirectory = std::move(templatePath);
                break;
            }

            // Check for source tree location (development builds)
            templatePath = searchPath + "/src/harmonyos/templates"_L1;
            if (options->verbose) {
                fprintf(stdout, "  Checking: %s ... %s\n", qPrintable(templatePath),
                       QDir(templatePath).exists() ? "FOUND" : "not found");
            }
            if (QDir(templatePath).exists()) {
                options->harmonyOsPackageSourceDirectory = std::move(templatePath);
                break;
            }

            // Move up one directory by removing last path component
            const auto lastSlash = searchPath.lastIndexOf('/'_L1);
            if (lastSlash <= 0) {
                if (options->verbose)
                    fprintf(stdout, "  Reached root directory\n");
                break;
            }
            searchPath.resize(lastSlash);
        }

        if (options->harmonyOsPackageSourceDirectory.isEmpty()) {
            fprintf(stderr, "Error: 'harmonyos-package-source-directory' not specified in JSON\n");
            fprintf(stderr, "       and could not auto-detect template location\n");
            fprintf(stderr, "       Please specify the path to the HarmonyOS application template\n");
            return false;
        } else if (options->verbose) {
            fprintf(stdout, "Auto-detected template: %s\n", qPrintable(options->harmonyOsPackageSourceDirectory));
        }
    }

    if (options->harmonyOsAppName.isEmpty()) {
        fprintf(stderr, "Error: 'harmonyos-app-name' not specified in JSON\n");
        return false;
    }

    if (options->harmonyOsAppBundleName.isEmpty()) {
        fprintf(stderr, "Error: 'harmonyos-app-bundle-name' not specified in JSON\n");
        return false;
    }

    // Set default output directory if not specified
    if (options->outputDirectory.isEmpty()) {
        if (options->testBundleMode) {
            options->outputDirectory = QDir::currentPath() + "/harmonyos-tests-bundle"_L1;
        } else {
            QFileInfo appInfo(options->applicationBinary);
            options->outputDirectory = QDir::currentPath() + "/"_L1 +
                                      appInfo.completeBaseName() + "-harmonyos"_L1;
        }
    }

    if (options->verbose) {
        fprintf(stdout, "Configuration loaded:\n");
        if (options->testBundleMode) {
            fprintf(stdout, "  Mode: test bundle\n");
            fprintf(stdout, "  Test binaries directory: %s\n", qPrintable(options->testBinariesDirectory));
            if (!options->testExcludeList.isEmpty())
                fprintf(stdout, "  Exclude list: %s\n", qPrintable(options->testExcludeList.join(", "_L1)));
        } else {
            fprintf(stdout, "  Application binary: %s\n", qPrintable(options->applicationBinary));
        }
        fprintf(stdout, "  Template directory: %s\n", qPrintable(options->harmonyOsPackageSourceDirectory));
        fprintf(stdout, "  App name: %s\n", qPrintable(options->harmonyOsAppName));
        fprintf(stdout, "  Bundle name: %s\n", qPrintable(options->harmonyOsAppBundleName));
        fprintf(stdout, "  Output directory: %s\n", qPrintable(options->outputDirectory));
        fprintf(stdout, "  Target architectures: %s\n", qPrintable(options->targetArchs.join(", "_L1)));
    }

    return true;
}

static bool copyFileIfNewer(const QString &sourceFileName,
                            const QString &destinationFileName, bool verbose,
                            bool forceOverwrite = false)
{
    if (QFile::exists(destinationFileName)) {
        QFileInfo destinationFileInfo(destinationFileName);
        QFileInfo sourceFileInfo(sourceFileName);

        // Skip if destination is same or newer (unless forcing overwrite)
        if (!forceOverwrite &&
            sourceFileInfo.lastModified() <= destinationFileInfo.lastModified()) {
            if (verbose)
                fprintf(stdout, "  Skipping: %s (destination is up to date)\n",
                        qPrintable(sourceFileInfo.fileName()));
            return true;
        }

        // Remove old file before copying
        if (!QFile(destinationFileName).remove()) {
            fprintf(stderr, "Failed to remove old file: %s\n",
                    qPrintable(destinationFileName));
            return false;
        }
    }

    // Ensure destination directory exists
    QFileInfo destInfo(destinationFileName);
    if (!QDir().mkpath(destInfo.absolutePath())) {
        fprintf(stderr, "Failed to create directory for: %s\n", qPrintable(destinationFileName));
        return false;
    }

    if (verbose)
        fprintf(stdout, "  Copying: %s\n", qPrintable(QFileInfo(sourceFileName).fileName()));

    QFile sourceFile(sourceFileName);
    if (!sourceFile.copy(destinationFileName)) {
        fprintf(stderr, "Failed to copy file: %s to %s: %s\n", qPrintable(sourceFileName),
                qPrintable(destinationFileName), qPrintable(sourceFile.errorString()));
        return false;
    }

    return true;
}

// Write dependency file for CMake DEPFILE support
static bool writeDepfile(const Options &options, const QString &hapOutputPath)
{
    if (options.depFilePath.isEmpty())
        return true; // Not requested

    if (options.verbose)
        fprintf(stdout, "Writing dependency file: %s\n", qPrintable(options.depFilePath));

    // Calculate relative HAP path from depfile base directory
    QString relativeHapPath;
    if (!options.depFileBase.isEmpty() && !hapOutputPath.isEmpty())
        relativeHapPath = QDir(options.depFileBase).relativeFilePath(hapOutputPath);
    else
        relativeHapPath = hapOutputPath;

    // Open depfile for writing
    QFile depFile(options.depFilePath);
    if (!depFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "Failed to open depfile: %s\n", qPrintable(options.depFilePath));
        return false;
    }

    // Write Makefile-style dependency format: target: dep1 \ dep2 \ ...
    depFile.write(escapeAndEncodeDependencyPath(relativeHapPath));
    depFile.write(": ");

    for (const QString &dep : dependenciesForDepfile) {
        depFile.write(" \\\n    ");
        depFile.write(escapeAndEncodeDependencyPath(dep));
    }

    depFile.write("\n");
    depFile.close();

    if (options.verbose)
        fprintf(stdout, "Wrote %lld dependencies to depfile\n",
                static_cast<long long>(dependenciesForDepfile.size()));

    return true;
}

static bool copyRecursively(const QString &sourceDir, const QString &destDir, bool verbose)
{
    QDir srcDir(sourceDir);
    if (!srcDir.exists()) {
        fprintf(stderr, "Source directory does not exist: %s\n", qPrintable(sourceDir));
        return false;
    }

    QDir destDirectory(destDir);
    if (!destDirectory.exists()) {
        if (!destDirectory.mkpath("."_L1)) {
            fprintf(stderr, "Failed to create destination directory: %s\n", qPrintable(destDir));
            return false;
        }
    }

    const QFileInfoList entries = srcDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const QFileInfo &entry : entries) {
        QString destPath = destDir + "/"_L1 + entry.fileName();

        if (entry.isDir()) {
            if (!copyRecursively(entry.filePath(), destPath, verbose))
                return false;
        } else {
            if (!copyFileIfNewer(entry.filePath(), destPath, verbose))
                return false;
        }
    }
    return true;
}

static bool copyTemplate(const Options &options)
{
    if (options.verbose) {
        fprintf(stdout, "Copying template from %s to %s\n",
               qPrintable(options.harmonyOsPackageSourceDirectory),
               qPrintable(options.outputDirectory));
    }

    // Check if template exists
    QDir templateDir(options.harmonyOsPackageSourceDirectory);
    if (!templateDir.exists()) {
        fprintf(stderr, "Template directory does not exist: %s\n",
               qPrintable(options.harmonyOsPackageSourceDirectory));
        return false;
    }

    // Create output directory
    QDir outputDir(options.outputDirectory);
    if (outputDir.exists()) {
        if (options.verbose)
            fprintf(stdout, "Output directory already exists, will overwrite files\n");
    }

    // Copy entire template
    if (!copyRecursively(options.harmonyOsPackageSourceDirectory, options.outputDirectory, options.verbose))
        return false;

    // Force-overwrite the manifest files. customizeTemplate() does one-shot
    // sentinel/regex substitutions on these; if the destination is left over
    // from a previous deploy the sentinels are already gone and any change to
    // the CMake-supplied metadata would be silently ignored. The same applies
    // to build-profile.json5: injectSigningConfig() looks for the empty
    // template array and refuses to touch anything else.
    for (const char *relPath : { "AppScope/app.json5", "entry/src/main/module.json5",
                                 "build-profile.json5" }) {
        const QString src = options.harmonyOsPackageSourceDirectory
            + QLatin1Char('/') + QLatin1String(relPath);
        const QString dst = options.outputDirectory + QLatin1Char('/') + QLatin1String(relPath);
        if (QFile::exists(src) && !copyFileIfNewer(src, dst, options.verbose, true))
            return false;
    }

    if (options.verbose)
        fprintf(stdout, "Template copied successfully\n");

    return true;
}

// Hvigor's module.json5 schema rejects free-form reason strings: it requires
// either a "$string:<id>" resource reference or a parameterised token (one
// containing both '{' and '}').  Plain English literals supplied via
// qt_add_harmonyos_permission(... REASON "...") therefore have to be
// auto-promoted to a synthesized resource entry before substitution.
static bool reasonNeedsPromotion(const QString &reason)
{
    if (reason.startsWith("$string:"_L1))
        return false;
    if (reason.contains(QLatin1Char('{')) && reason.contains(QLatin1Char('}')))
        return false;
    return true;
}

// Synthesize a stable resource id from a permission name. The trailing
// dot-separated component is unique among ohos.permission.* permissions, so
// "ohos.permission.CAMERA" -> "qt_permission_reason_camera".
static QString synthesizePermissionReasonId(const QString &permissionName)
{
    const QString suffix = permissionName.section(QLatin1Char('.'), -1).toLower();
    return "qt_permission_reason_"_L1 + suffix;
}

// scalar. User-supplied metadata (vendor, label, etc.) is substituted into
// the OHOS manifest files verbatim, so embedded '"' or '\' would otherwise
// produce invalid JSON that hvigor rejects. Re-uses Qt's own JSON writer
// for the canonical escape: wrap in a single-element array, serialize, strip
// the surrounding `["` and `"]`.
static QString jsonStringEscape(const QString &s)
{
    QByteArray ba = QJsonDocument(QJsonArray{s}).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(ba.sliced(2, ba.size() - 4));
}

// Returns true when value is one of the orientation strings the HarmonyOS
// module.json5 schema accepts. Anything else gets rejected with a warning and
// dropped, so a typo never reaches hvigor (which would fail with a less
// targeted schema error).
static bool isValidHarmonyOsAbilityOrientation(const QString &value)
{
    static const QStringList allowed = {
        "unspecified"_L1,
        "landscape"_L1,
        "portrait"_L1,
        "follow_recent"_L1,
        "landscape_inverted"_L1,
        "portrait_inverted"_L1,
        "auto_rotation"_L1,
        "auto_rotation_landscape"_L1,
        "auto_rotation_portrait"_L1,
        "auto_rotation_restricted"_L1,
        "auto_rotation_landscape_restricted"_L1,
        "auto_rotation_portrait_restricted"_L1,
        "locked"_L1,
        "follow_desktop"_L1,
    };
    return allowed.contains(value);
}

struct PromotedReason
{
    QString id;
    QString value;
};

static bool customizeTemplate(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Customizing template files\n");

    // Customize QtAppConstants.ets
    QString qtAppConstantsPath = options.outputDirectory + "/entry/src/main/ets/common/QtAppConstants.ets"_L1;
    QFile qtAppConstantsFile(qtAppConstantsPath);

    if (qtAppConstantsFile.exists()) {
        if (!qtAppConstantsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fprintf(stderr, "Failed to open QtAppConstants.ets for reading\n");
            return false;
        }

        QString content = QString::fromUtf8(qtAppConstantsFile.readAll());
        qtAppConstantsFile.close();

        // Replace APP_LIBRARY_NAME
        // In test bundle mode, use a placeholder — runtime override selects the actual test
        QString appLibName;
        if (options.testBundleMode) {
            appLibName = "libtst_placeholder.so"_L1;
        } else {
            QFileInfo appInfo(options.applicationBinary);
            appLibName = appInfo.fileName(); // Keep the full filename with lib prefix and .so extension
        }

        content.replace(QRegularExpression("APP_LIBRARY_NAME = '[^']*'"_L1),
                       "APP_LIBRARY_NAME = '"_L1 + appLibName + "'"_L1);

        if (!qtAppConstantsFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            fprintf(stderr, "Failed to open QtAppConstants.ets for writing\n");
            return false;
        }

        qtAppConstantsFile.write(content.toUtf8());
        qtAppConstantsFile.close();

        if (options.verbose)
            fprintf(stdout, "  Updated QtAppConstants.ets with app name: %s\n", qPrintable(appLibName));
    }

    // Resolve the icon value once -- it is consumed by both the app.json5
    // (app-level icon) and module.json5 (ability/launcher icon) customizations
    // below. $media: references pass through; literal filesystem paths get
    // copied into the AppScope *and* entry resource dirs and rewritten to a
    // $media:<basename> reference. OHOS restool restricts resource names to
    // [a-zA-Z0-9_]; sanitize the basename so files like "qt-logo.png" are
    // accepted (becomes "qt_logo.png" / $media:qt_logo).
    QString iconValue = options.harmonyOsAppIcon;
    if (!iconValue.isEmpty() && !iconValue.startsWith("$media:"_L1)) {
        QFileInfo iconInfo(iconValue);
        if (!iconInfo.exists() || !iconInfo.isFile()) {
            fprintf(stderr, "App icon does not exist: %s\n", qPrintable(iconValue));
            return false;
        }
        QString safeStem = iconInfo.completeBaseName();
        for (QChar &c : safeStem) {
            if (!c.isLetterOrNumber() && c != QLatin1Char('_'))
                c = QLatin1Char('_');
        }
        const QString destFileName = iconInfo.suffix().isEmpty()
                ? safeStem
                : safeStem + "."_L1 + iconInfo.suffix();
        const QStringList destDirs = {
            options.outputDirectory + "/AppScope/resources/base/media"_L1,
            options.outputDirectory + "/entry/src/main/resources/base/media"_L1,
        };
        for (const QString &destDir : destDirs) {
            QDir().mkpath(destDir);
            const QString destPath = destDir + "/"_L1 + destFileName;
            if (!copyFileIfNewer(iconValue, destPath, options.verbose)) {
                fprintf(stderr, "Failed to copy app icon to: %s\n", qPrintable(destPath));
                return false;
            }
        }
        iconValue = "$media:"_L1 + safeStem;
    }

    // Customize AppScope/app.json5. Use targeted text substitution rather
    // than parse-and-rewrite so JSON5 features in the template (comments,
    // trailing commas, single-quoted strings) survive the round trip --
    // QJsonDocument is a strict JSON parser and would reject those.
    QString appJsonPath = options.outputDirectory + "/AppScope/app.json5"_L1;
    QFile appJsonFile(appJsonPath);

    if (appJsonFile.exists()) {
        if (!appJsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fprintf(stderr, "Failed to open app.json5 for reading\n");
            return false;
        }
        QString content = QString::fromUtf8(appJsonFile.readAll());
        appJsonFile.close();

        auto replaceStringField =
                [&content](QLatin1StringView key, const QString &value) {
            if (value.isEmpty())
                return;
            content.replace(
                    QRegularExpression("\""_L1 + key + "\":\\s*\"[^\"]*\""_L1),
                    "\""_L1 + key + "\": \""_L1 + jsonStringEscape(value) + "\""_L1);
        };

        replaceStringField("bundleName"_L1, options.harmonyOsAppBundleName);
        replaceStringField("vendor"_L1, options.harmonyOsAppVendor);
        replaceStringField("versionName"_L1, options.harmonyOsAppVersionName);
        // The OHOS schema for app.label requires either "$string:<id>" or a
        // brace-substituted value -- a plain literal is rejected. So only
        // substitute the label field directly when the user supplied a
        // $string: reference. Literal labels are routed below to the
        // app_name/QAbility_label string resources, which app.json5 and
        // module.json5 already reference via $string:.
        if (options.harmonyOsAppLabel.startsWith("$string:"_L1))
            replaceStringField("label"_L1, options.harmonyOsAppLabel);
        replaceStringField("icon"_L1, iconValue);

        if (options.harmonyOsAppVersionCode > 0) {
            content.replace(
                    QRegularExpression("\"versionCode\":\\s*\\d+"_L1),
                    "\"versionCode\": "_L1 + QString::number(options.harmonyOsAppVersionCode));
        }

        if (!appJsonFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            fprintf(stderr, "Failed to open app.json5 for writing\n");
            return false;
        }
        appJsonFile.write(content.toUtf8());
        appJsonFile.close();

        if (options.verbose) {
            fprintf(stdout, "  Updated app.json5 (bundle: %s)\n",
                    qPrintable(options.harmonyOsAppBundleName));
        }
    }

    // Display label that lands in the app_name and QAbility_label string
    // resources. A literal LABEL wins; a "$string:..." LABEL was substituted
    // into app.json5 above and is therefore the user's own resource id, so we
    // fall back to the target name here.
    const QString displayLabel =
        (!options.harmonyOsAppLabel.isEmpty()
         && !options.harmonyOsAppLabel.startsWith("$string:"_L1))
            ? options.harmonyOsAppLabel
            : options.harmonyOsAppName;

    // Auto-promote plain-literal permission reasons to $string: references.
    // The literal values are appended to the entry/.../string.json files below
    // so the synthesized resource ids resolve correctly at HAP build time.
    QJsonArray transformedPermissions;
    QList<PromotedReason> promotedReasons;
    QSet<QString> seenPromotedIds;
    for (const QJsonValue &value : std::as_const(options.permissions)) {
        if (!value.isObject()) {
            transformedPermissions.append(value);
            continue;
        }
        QJsonObject entry = value.toObject();
        if (entry.contains("reason"_L1)) {
            const QString reason = entry["reason"_L1].toString();
            if (reasonNeedsPromotion(reason)) {
                const QString permName = entry["name"_L1].toString();
                const QString stringId = synthesizePermissionReasonId(permName);
                entry["reason"_L1] = QString("$string:"_L1 + stringId);
                if (!seenPromotedIds.contains(stringId)) {
                    promotedReasons.append({stringId, reason});
                    seenPromotedIds.insert(stringId);
                }
            }
        }
        transformedPermissions.append(entry);
    }

    // Customize entry module string resources: replace QAbility_label with the
    // app name, and append any synthesized permission-reason strings.
    // Update all locale variants: base, en_US, zh_CN
    QStringList locales = QStringList() << "base"_L1 << "en_US"_L1 << "zh_CN"_L1;

    for (const QString &locale : locales) {
        QString stringJsonPath = options.outputDirectory + "/entry/src/main/resources/"_L1 +
                                locale + "/element/string.json"_L1;
        QFile stringJsonFile(stringJsonPath);

        if (!stringJsonFile.exists())
            continue;

        if (!stringJsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fprintf(stderr, "Failed to open %s string.json for reading\n", qPrintable(locale));
            continue;
        }

        const QByteArray bytes = stringJsonFile.readAll();
        stringJsonFile.close();

        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            fprintf(stderr, "Failed to parse %s string.json: %s\n",
                    qPrintable(locale), qPrintable(parseErr.errorString()));
            continue;
        }
        QJsonObject root = doc.object();
        QJsonArray strings = root["string"_L1].toArray();

        // Replace QAbility_label value with app name and collect existing names
        QSet<QString> existingNames;
        for (qsizetype i = 0; i < strings.size(); ++i) {
            QJsonObject e = strings[i].toObject();
            const QString name = e["name"_L1].toString();
            existingNames.insert(name);
            if (name == "QAbility_label"_L1) {
                e["value"_L1] = displayLabel;
                strings[i] = e;
            }
        }

        // Append synthesized permission-reason strings (skip ids already present)
        for (const PromotedReason &p : std::as_const(promotedReasons)) {
            if (existingNames.contains(p.id))
                continue;
            QJsonObject e;
            e["name"_L1] = p.id;
            e["value"_L1] = p.value;
            strings.append(e);
            existingNames.insert(p.id);
        }
        root["string"_L1] = strings;
        doc.setObject(root);

        if (!stringJsonFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            fprintf(stderr, "Failed to open %s string.json for writing\n", qPrintable(locale));
            continue;
        }

        stringJsonFile.write(doc.toJson(QJsonDocument::Indented));
        stringJsonFile.close();

        if (options.verbose) {
            fprintf(stdout,
                    "  Updated %s string.json (label: %s, +%lld promoted permission reasons)\n",
                    qPrintable(locale), qPrintable(displayLabel),
                    static_cast<long long>(promotedReasons.size()));
        }
    }

    // Also update AppScope app_name for consistency
    QString appScopeStringPath = options.outputDirectory + "/AppScope/resources/base/element/string.json"_L1;
    QFile appScopeStringFile(appScopeStringPath);

    if (appScopeStringFile.exists()) {
        if (!appScopeStringFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fprintf(stderr, "Failed to open AppScope string.json for reading\n");
            return false;
        }

        QString content = QString::fromUtf8(appScopeStringFile.readAll());
        appScopeStringFile.close();

        // Replace app_name value
        QRegularExpression appNameRegex("(\"name\":\\s*\"app_name\"[^}]*\"value\":\\s*)\"[^\"]*\""_L1);
        content.replace(appNameRegex, "\\1\""_L1 + jsonStringEscape(displayLabel) + "\""_L1);

        if (!appScopeStringFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            fprintf(stderr, "Failed to open AppScope string.json for writing\n");
            return false;
        }

        appScopeStringFile.write(content.toUtf8());
        appScopeStringFile.close();

        if (options.verbose)
            fprintf(stdout, "  Updated AppScope string.json with app name: %s\n",
                   qPrintable(displayLabel));
    }

    // Customize module.json5
    // Note: We only update the description, not the module name which must remain "entry"
    QString moduleJsonPath = options.outputDirectory + "/entry/src/main/module.json5"_L1;
    QFile moduleJsonFile(moduleJsonPath);

    if (moduleJsonFile.exists()) {
        if (!moduleJsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            fprintf(stderr, "Failed to open module.json5 for reading\n");
            return false;
        }

        QString content = QString::fromUtf8(moduleJsonFile.readAll());
        moduleJsonFile.close();

        // Substitute the module description sentinel. The user's value (if set
        // via QT_HARMONYOS_MODULE_DESCRIPTION) takes precedence;
        // otherwise fall back to the template's $string:module_desc reference,
        // which resolves via the entry/.../resources/.../string.json file.
        const QString descriptionSentinel = "%%INSERT_MODULE_DESCRIPTION%%"_L1;
        const QString descriptionValue = options.harmonyOsModuleDescription.isEmpty()
            ? "$string:module_desc"_L1
            : jsonStringEscape(options.harmonyOsModuleDescription);
        content.replace(descriptionSentinel, descriptionValue);

        // Substitute the deviceTypes sentinel.
        const QString deviceTypesSentinel = "/* %%INSERT_DEVICE_TYPES%% */"_L1;
        QStringList deviceTypes = options.harmonyOsModuleDeviceTypes;
        if (deviceTypes.isEmpty())
            deviceTypes = QStringList{ "phone"_L1, "tablet"_L1, "2in1"_L1 };
        QStringList quotedDeviceTypes;
        quotedDeviceTypes.reserve(deviceTypes.size());
        for (const QString &dt : std::as_const(deviceTypes))
            quotedDeviceTypes.append("\""_L1 + dt + "\""_L1);
        content.replace(deviceTypesSentinel, quotedDeviceTypes.join(", "_L1));

        // Substitute the ability-orientation sentinel. The template ships the
        // sentinel as a block comment so module.json5 stays valid JSON5 when
        // the user has not set an orientation; in that case the sentinel line
        // is dropped entirely. When set, replace it with the orientation field
        // (matching the surrounding 8-space indentation already in the
        // template). Unknown values are rejected with a warning rather than
        // forwarded to hvigor, which would fail with a less targeted error.
        const QString orientationSentinelLine =
                "        /* %%INSERT_ABILITY_ORIENTATION%% */\n"_L1;
        QString orientationReplacement;
        if (!options.harmonyOsAbilityOrientation.isEmpty()) {
            if (isValidHarmonyOsAbilityOrientation(options.harmonyOsAbilityOrientation)) {
                orientationReplacement = "        \"orientation\": \""_L1
                        + options.harmonyOsAbilityOrientation
                        + "\",\n"_L1;
            } else {
                fprintf(stderr,
                        "Warning: Ignoring unknown harmonyos-ability-orientation value '%s'\n",
                        qPrintable(options.harmonyOsAbilityOrientation));
            }
        }
        content.replace(orientationSentinelLine, orientationReplacement);

        // Override the ability/launcher icon so QT_HARMONYOS_APP_ICON
        // is reflected on the device home screen, not just in Settings. The
        // template ships "$media:layered_image" -- only replace that specific
        // value so user-customized icons in subsequent runs aren't clobbered.
        if (!iconValue.isEmpty()) {
            content.replace(
                    QRegularExpression("\"icon\":\\s*\"\\$media:layered_image\""_L1),
                    "\"icon\": \""_L1 + iconValue + "\""_L1);
        }

        // Build the requestPermissions array fragment from the (possibly
        // promoted) transformedPermissions computed above.  The sentinel
        // "/* %%INSERT_PERMISSIONS%% */" sits inside an empty [] so the
        // template stays valid JSON5 even without substitution.
        const QString sentinel = "/* %%INSERT_PERMISSIONS%% */"_L1;
        QString permissionsFragment;
        if (!transformedPermissions.isEmpty()) {
            QStringList entryStrings;
            entryStrings.reserve(transformedPermissions.size());
            for (const QJsonValue &value : std::as_const(transformedPermissions)) {
                if (!value.isObject())
                    continue;
                const QJsonObject entry = value.toObject();

                // Pretty-print the entry, then re-indent so it lines up with the
                // surrounding "requestPermissions" array (8-space base indent).
                const QByteArray pretty =
                    QJsonDocument(entry).toJson(QJsonDocument::Indented).trimmed();
                const QStringList lines = QString::fromUtf8(pretty).split(QLatin1Char('\n'));
                QStringList indented;
                indented.reserve(lines.size());
                for (const QString &line : lines)
                    indented.append("        "_L1 + line);
                entryStrings.append(indented.join(QLatin1Char('\n')));
            }
            if (!entryStrings.isEmpty()) {
                permissionsFragment = "\n"_L1 + entryStrings.join(",\n"_L1) + "\n    "_L1;
            }
        }
        content.replace(sentinel, permissionsFragment);

        if (!moduleJsonFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            fprintf(stderr, "Failed to open module.json5 for writing\n");
            return false;
        }

        moduleJsonFile.write(content.toUtf8());
        moduleJsonFile.close();

        if (options.verbose) {
            fprintf(stdout, "  Updated module.json5 description\n");
            fprintf(stdout, "  Injected %lld permissions into module.json5\n",
                    static_cast<long long>(transformedPermissions.size()));
        }
    }

    // Customize build-profile.json5 with the SDK version metadata. Only fields
    // the user explicitly set are substituted; others keep the template default.
    //
    //   * compatibleSdkVersion is an existing key with a default value -- we
    //     replace its value via the same regex pattern used in app.json5.
    //   * targetSdkVersion / compileSdkVersion don't appear in the template by
    //     default. They are added via comment-style sentinels that the JSON5
    //     parser ignores when not substituted.
    {
        const bool anySdkVersionSet =
            !options.harmonyOsCompatibleSdkVersion.isEmpty()
            || !options.harmonyOsTargetSdkVersion.isEmpty()
            || !options.harmonyOsCompileSdkVersion.isEmpty();

        const QString buildProfilePath =
            options.outputDirectory + "/build-profile.json5"_L1;
        QFile buildProfileFile(buildProfilePath);
        if (anySdkVersionSet && buildProfileFile.exists()) {
            if (!buildProfileFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                fprintf(stderr, "Failed to open build-profile.json5 for reading\n");
                return false;
            }
            QString content = QString::fromUtf8(buildProfileFile.readAll());
            buildProfileFile.close();

            if (!options.harmonyOsCompatibleSdkVersion.isEmpty()) {
                content.replace(
                    QRegularExpression(
                        "\"compatibleSdkVersion\":\\s*\"[^\"]*\""_L1),
                    "\"compatibleSdkVersion\": \""_L1
                        + jsonStringEscape(options.harmonyOsCompatibleSdkVersion)
                        + "\""_L1);
            }

            const QString targetSentinel = "/* %%INSERT_TARGET_SDK_VERSION%% */"_L1;
            const QString targetReplacement =
                options.harmonyOsTargetSdkVersion.isEmpty()
                    ? QString()
                    : "\"targetSdkVersion\": \""_L1
                          + jsonStringEscape(options.harmonyOsTargetSdkVersion)
                          + "\","_L1;
            content.replace(targetSentinel, targetReplacement);

            const QString compileSentinel = "/* %%INSERT_COMPILE_SDK_VERSION%% */"_L1;
            const QString compileReplacement =
                options.harmonyOsCompileSdkVersion.isEmpty()
                    ? QString()
                    : "\"compileSdkVersion\": \""_L1
                          + jsonStringEscape(options.harmonyOsCompileSdkVersion)
                          + "\","_L1;
            content.replace(compileSentinel, compileReplacement);

            if (!buildProfileFile.open(
                    QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                fprintf(stderr, "Failed to open build-profile.json5 for writing\n");
                return false;
            }
            buildProfileFile.write(content.toUtf8());
            buildProfileFile.close();

            if (options.verbose) {
                fprintf(stdout,
                        "  Updated build-profile.json5 SDK versions"
                        " (compatible=%s, target=%s, compile=%s)\n",
                        qPrintable(options.harmonyOsCompatibleSdkVersion),
                        qPrintable(options.harmonyOsTargetSdkVersion),
                        qPrintable(options.harmonyOsCompileSdkVersion));
            }
        }
    }

    if (options.verbose)
        fprintf(stdout, "Template customization completed\n");

    return true;
}

static bool copyApplicationBinary(const Options &options)
{
    if (options.verbose)
        fprintf(stdout, "Copying application binary and dependencies\n");

    // For each target architecture, copy the application binary
    for (const QString &arch : options.targetArchs) {
        QString archLibPath = options.outputDirectory + "/entry/libs/"_L1 + arch;
        QDir archDir(archLibPath);
        if (!archDir.exists()) {
            if (!archDir.mkpath("."_L1)) {
                fprintf(stderr, "Failed to create architecture directory: %s\n", qPrintable(archLibPath));
                return false;
            }
        }

        // Copy application binary
        QFileInfo appInfo(options.applicationBinary);
        QString destPath = archLibPath + "/"_L1 + appInfo.fileName();

        if (!appInfo.fileName().startsWith("lib"_L1)) {
            // Ensure it has lib prefix
            destPath = archLibPath + "/lib"_L1 + appInfo.fileName();
        }

        if (!destPath.endsWith(".so"_L1)) {
            // Ensure it has .so extension
            destPath += ".so"_L1;
        }

        if (options.verbose) {
            fprintf(stdout, "  Copying application binary for %s: %s -> %s\n",
                   qPrintable(arch), qPrintable(options.applicationBinary), qPrintable(destPath));
        }

        if (!copyFileIfNewer(options.applicationBinary, destPath, options.verbose)) {
          fprintf(stderr, "Failed to copy application binary to: %s\n",
                  qPrintable(destPath));
          return false;
        }

        // Track as dependency for depfile
        if (!options.depFilePath.isEmpty())
            dependenciesForDepfile << options.applicationBinary;
    }

    if (options.verbose)
        fprintf(stdout, "Application binary copied successfully\n");

    return true;
}

static bool copyFileToArchitectures(const Options &options,
                                    const QString &sourcePath,
                                    const QString &relativeDestPath,
                                    bool trackInDepfile = true)
{
    for (const QString &arch : options.targetArchs) {
        QString destPath = "%1/entry/libs/%2/%3"_L1
            .arg(options.outputDirectory, arch, relativeDestPath);

        QDir().mkpath(QFileInfo(destPath).absolutePath());

        if (options.verbose)
            fprintf(stdout, "  Copying for %s: %s\n",
                   qPrintable(arch), qPrintable(QFileInfo(sourcePath).fileName()));

        if (!copyFileIfNewer(sourcePath, destPath, options.verbose)) {
            fprintf(stderr, "Failed to copy file: %s to %s\n",
                   qPrintable(sourcePath), qPrintable(destPath));
            return false;
        }

        // Track as dependency for depfile (only once, not per-arch)
        if (trackInDepfile && !options.depFilePath.isEmpty() && arch == options.targetArchs.first())
            dependenciesForDepfile << sourcePath;
    }
    return true;
}

static QString findStdCppLibrary(const Options &options, const QString &arch)
{
    // Map architecture to NDK triple
    QString ndkArch;
    if (arch == "arm64-v8a"_L1) {
        ndkArch = "aarch64-linux-ohos"_L1;
    } else if (arch == "armeabi-v7a"_L1) {
        ndkArch = "arm-linux-ohos"_L1;
    } else if (arch == "x86_64"_L1) {
        ndkArch = "x86_64-linux-ohos"_L1;
    } else if (arch == "x86"_L1) {
        ndkArch = "i686-linux-ohos"_L1;
    } else {
        return QString();
    }

    QString stdCppPath = options.ndkRoot + "/llvm/lib/"_L1 + ndkArch + "/c++/libc++_shared.so"_L1;
    if (QFile::exists(stdCppPath))
        return stdCppPath;

    stdCppPath = options.ndkRoot + "/llvm/lib/"_L1 + ndkArch + "/libc++_shared.so"_L1;
    if (QFile::exists(stdCppPath))
        return stdCppPath;

    return QString();
}

// Copy project-specific shared libraries (test helper libs, etc.) flat into
// entry/libs/<arch>/. These are non-Qt libs listed in "project-libraries" in
// the deployment settings JSON, collected from the target's LINK_LIBRARIES by
// Qt6HarmonyOSMacros.cmake.
static bool copyProjectLibraries(const Options &options)
{
    if (options.projectLibraries.isEmpty())
        return true;

    if (options.verbose)
        fprintf(stdout, "Copying project libraries\n");

    for (const QString &projectLib : options.projectLibraries) {
        QFileInfo libInfo(projectLib);
        if (!libInfo.exists()) {
            if (options.verbose)
                fprintf(stdout, "  Project library not found, skipping: %s\n",
                        qPrintable(projectLib));
            continue;
        }

        for (const QString &arch : options.targetArchs) {
            QString archLibPath = options.outputDirectory + "/entry/libs/"_L1 + arch;
            QString destPath = archLibPath + "/"_L1 + libInfo.fileName();

            if (options.verbose) {
                fprintf(stdout, "  Copying project library for %s: %s\n",
                       qPrintable(arch), qPrintable(libInfo.fileName()));
            }

            if (!copyFileIfNewer(projectLib, destPath, options.verbose)) {
                fprintf(stderr, "Failed to copy project library to: %s\n",
                        qPrintable(destPath));
                return false;
            }

            if (!options.depFilePath.isEmpty())
                dependenciesForDepfile << projectLib;
        }
    }

    if (options.verbose)
        fprintf(stdout, "Project libraries copied successfully\n");

    return true;
}

// Recursively scan dirPath for libtst_*.so test binaries and their co-located helper libs.
// Test binaries are appended to found; helper libs (lib*.so* in the same directory as a
// test binary) are appended to foundHelpers. helperNames guards against filename collisions
// across directories. excludeDirs is a list of absolute paths to skip during recursion.
static void scanTestBinariesDir(const QString &dirPath,
                                const QStringList &excludeList,
                                const QStringList &excludeDirs,
                                QStringList &found,
                                QStringList &foundHelpers,
                                QSet<QString> &helperNames)
{
    const QFileInfoList entries =
        QDir(dirPath).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    bool hasTestBinary = false;
    for (const QFileInfo &entry : entries) {
        if (!entry.isDir()
            && entry.fileName().startsWith("libtst_"_L1)
            && entry.suffix() == "so"_L1
            && !excludeList.contains(entry.fileName())) {
            found.append(entry.filePath());
            hasTestBinary = true;
        }
    }

    // For each directory that contains a test binary, also collect co-located helper libs
    // (e.g. libqmetatype_lib1.so.0) so that $ORIGIN rpath lookups resolve on-device.
    if (hasTestBinary) {
        for (const QFileInfo &entry : entries) {
            if (!entry.isDir()
                && entry.fileName().startsWith("lib"_L1)
                && !entry.fileName().startsWith("libtst_"_L1)
                && entry.fileName().contains(".so"_L1)
                && !helperNames.contains(entry.fileName())) {
                foundHelpers.append(entry.filePath());
                helperNames.insert(entry.fileName());
            }
        }
    }

    for (const QFileInfo &entry : entries) {
        if (entry.isDir() && !excludeDirs.contains(entry.absoluteFilePath()))
            scanTestBinariesDir(entry.filePath(), excludeList, excludeDirs, found, foundHelpers, helperNames);
    }
}

static bool copyTestBinaries(const Options &options, QStringList &bundledBinaries)
{
    if (options.testBinariesDirectory.isEmpty()) {
        fprintf(stderr, "Error: 'test-binaries-directory' not specified for test bundle mode\n");
        return false;
    }

    if (!QDir(options.testBinariesDirectory).exists()) {
        fprintf(stderr, "Error: test-binaries-directory does not exist: %s\n",
                qPrintable(options.testBinariesDirectory));
        return false;
    }

    if (options.verbose)
        fprintf(stdout, "Scanning for test binaries in %s\n", qPrintable(options.testBinariesDirectory));

    QStringList found;
    QStringList foundHelpers;
    QSet<QString> helperNames;
    // Exclude the output directory to avoid scanning previously generated HAP bundle contents,
    // which would cause libentry.so (built by hvigor/CMake) to be picked up as a helper lib
    // and then conflict with the CMake-built version during the hvigor build.
    const QStringList excludeDirs = { QFileInfo(options.outputDirectory).absoluteFilePath() };
    scanTestBinariesDir(options.testBinariesDirectory, options.testExcludeList, excludeDirs,
                        found, foundHelpers, helperNames);

    if (found.isEmpty()) {
        fprintf(stderr, "Warning: No test binaries (libtst_*.so) found in %s\n",
                qPrintable(options.testBinariesDirectory));
        return true; // Not fatal
    }

    if (options.verbose) {
        fprintf(stdout, "Found %lld test binaries\n", static_cast<long long>(found.size()));
        if (!foundHelpers.isEmpty())
            fprintf(stdout, "Found %lld test helper libraries\n",
                    static_cast<long long>(foundHelpers.size()));
    }

    // Copy all test binaries AND helper libs flat to entry/libs/${arch}/
    for (const QString &arch : options.targetArchs) {
        QString archLibPath = options.outputDirectory + "/entry/libs/"_L1 + arch;
        QDir().mkpath(archLibPath);

        for (const QString &testBinary : found) {
            QFileInfo testInfo(testBinary);
            QString destPath = archLibPath + "/"_L1 + testInfo.fileName();

            if (options.verbose)
                fprintf(stdout, "  Copying test binary: %s\n", qPrintable(testInfo.fileName()));

            if (!copyFileIfNewer(testBinary, destPath, options.verbose)) {
                fprintf(stderr, "Failed to copy test binary: %s\n", qPrintable(testBinary));
                return false;
            }
        }

        for (const QString &helperLib : foundHelpers) {
            QFileInfo helperInfo(helperLib);
            QString destPath = archLibPath + "/"_L1 + helperInfo.fileName();

            if (options.verbose)
                fprintf(stdout, "  Copying test helper lib: %s\n", qPrintable(helperInfo.fileName()));

            if (!copyFileIfNewer(helperLib, destPath, options.verbose)) {
                fprintf(stderr, "Failed to copy test helper lib: %s\n", qPrintable(helperLib));
                return false;
            }
        }
    }

    // Build list of bundled binary filenames (no duplicates)
    for (const QString &testBinary : found) {
        QString fileName = QFileInfo(testBinary).fileName();
        if (!bundledBinaries.contains(fileName))
            bundledBinaries.append(fileName);
    }

    // Track for depfile
    if (!options.depFilePath.isEmpty()) {
        for (const QString &testBinary : found)
            dependenciesForDepfile << testBinary;
        for (const QString &helperLib : foundHelpers)
            dependenciesForDepfile << helperLib;
    }

    return true;
}

static bool writeTestBinariesList(const Options &options, const QStringList &bundledBinaries)
{
    QString binariesListPath = options.outputDirectory + "/binaries.txt"_L1;

    if (options.verbose)
        fprintf(stdout, "Writing test binaries list: %s\n", qPrintable(binariesListPath));

    QFile file(binariesListPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "Failed to open binaries.txt for writing: %s\n",
                qPrintable(binariesListPath));
        return false;
    }

    for (const QString &binary : bundledBinaries) {
        file.write(binary.toUtf8());
        file.write("\n");
    }
    file.close();

    if (options.verbose) {
        fprintf(stdout, "Wrote %lld test binary names to binaries.txt\n",
                static_cast<long long>(bundledBinaries.size()));
    }

    return true;
}

static QString readElfSoname(const Options &options, const QString &binaryPath);

static bool copyAllQtLibs(const Options &options)
{
    if (options.qtLibsDirectory.isEmpty())
        return true;

    if (!QDir(options.qtLibsDirectory).exists()) {
        if (options.verbose) {
            fprintf(stdout, "Qt libs directory not found, skipping: %s\n",
                    qPrintable(options.qtLibsDirectory));
        }
        return true;
    }

    if (options.verbose)
        fprintf(stdout, "Copying all Qt libraries from %s\n", qPrintable(options.qtLibsDirectory));

    QDir libsDir(options.qtLibsDirectory);
    const QFileInfoList entries = libsDir.entryInfoList({"*.so"_L1, "*.so.*"_L1}, QDir::Files);

    for (const QFileInfo &entry : entries) {
        for (const QString &arch : options.targetArchs) {
            QString destPath = options.outputDirectory + "/entry/libs/"_L1 + arch + "/"_L1 + entry.fileName();
            QDir().mkpath(QFileInfo(destPath).absolutePath());
            if (!copyFileIfNewer(entry.filePath(), destPath, options.verbose))
                return false;
        }
        if (!options.depFilePath.isEmpty())
            dependenciesForDepfile << entry.filePath();
    }

    // Also copy libc++_shared.so from NDK for each architecture
    for (const QString &arch : options.targetArchs) {
        QString stdCppPath = findStdCppLibrary(options, arch);
        if (!stdCppPath.isEmpty()) {
            QString destPath = options.outputDirectory + "/entry/libs/"_L1 + arch + "/libc++_shared.so"_L1;
            if (options.verbose)
                fprintf(stdout, "  Copying C++ standard library for %s\n", qPrintable(arch));
            if (!copyFileIfNewer(stdCppPath, destPath, options.verbose))
                return false;
            if (!options.depFilePath.isEmpty())
                dependenciesForDepfile << stdCppPath;
        }
    }

    // Copy all libraries from extra-libs-dirs (e.g. third-party deps like ICU, fontconfig)
    for (const QString &extraDir : options.extraLibsDirs) {
        QDir dir(extraDir);
        if (!dir.exists()) {
            if (options.verbose)
                fprintf(stdout, "Extra libs dir not found, skipping: %s\n", qPrintable(extraDir));
            continue;
        }

        if (options.verbose)
            fprintf(stdout, "Copying extra libraries from %s\n", qPrintable(extraDir));

        const QFileInfoList entries = dir.entryInfoList({"*.so"_L1, "*.so.*"_L1}, QDir::Files);
        for (const QFileInfo &entry : entries) {
            // Deploy using the library's SONAME if it differs from the on-disk filename,
            // so the dynamic linker can find it at runtime (e.g. libicudata.so.78,
            // not libicudata.so).
            const QString soname = readElfSoname(options, entry.filePath());
            const QString deployName = (!soname.isEmpty() && soname != entry.fileName())
                                           ? soname : entry.fileName();
            for (const QString &arch : options.targetArchs) {
                QString destPath = options.outputDirectory + "/entry/libs/"_L1 + arch + "/"_L1 + deployName;
                QDir().mkpath(QFileInfo(destPath).absolutePath());
                if (!copyFileIfNewer(entry.filePath(), destPath, options.verbose))
                    return false;
            }
            if (!options.depFilePath.isEmpty())
                dependenciesForDepfile << entry.filePath();
        }
    }

    return true;
}

static bool copyAllQtPlugins(const Options &options)
{
    // Copy all plugins from one plugins root directory.
    auto copyPluginsFromDir = [&options](const QString &pluginsRootPath) -> bool {
        QDir pluginsDir(pluginsRootPath);
        const QStringList categories = pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (const QString &category : categories) {
            QDir categoryDir(pluginsDir.filePath(category));
            const QFileInfoList plugins = categoryDir.entryInfoList({"*.so"_L1}, QDir::Files);

            for (const QFileInfo &pluginInfo : plugins) {
                const QString &plugin = pluginInfo.fileName();
                const QString &pluginPath = pluginInfo.filePath();

                if (category == "platforms"_L1 && plugin == "libqohos.so"_L1) {
                    // Platform plugin goes flat to root libs directory
                    for (const QString &arch : options.targetArchs) {
                        QString destPath = options.outputDirectory + "/entry/libs/"_L1 + arch + "/libqohos.so"_L1;
                        QDir().mkpath(QFileInfo(destPath).absolutePath());
                        if (!copyFileIfNewer(pluginPath, destPath, options.verbose))
                            return false;
                    }
                } else {
                    // All other plugins go into their category subdirectory
                    QString relativeDestPath = category + "/"_L1 + plugin;
                    if (!copyFileToArchitectures(options, pluginPath, relativeDestPath, false))
                        return false;
                }

                if (!options.depFilePath.isEmpty())
                    dependenciesForDepfile << pluginPath;
            }
        }
        return true;
    };

    // Process plugins-import-paths first (typically CMAKE_BINARY_DIR/plugins,
    // i.e. the module's own build-tree plugin output).  Files written here
    // receive dest mtime = now, so the subsequent qtPluginsDirectory pass
    // skips any file that was already copied — this gives build-dir contents
    // unconditional priority over the installed Qt prefix without requiring a
    // force-overwrite flag.
    for (const QString &importPath : options.pluginsImportPaths) {
        if (!QDir(importPath).exists()) {
            if (options.verbose)
                fprintf(stdout, "Plugins import path not found, skipping: %s\n",
                        qPrintable(importPath));
            continue;
        }
        if (options.verbose)
            fprintf(stdout, "Copying Qt plugins from import path: %s\n",
                    qPrintable(importPath));
        if (!copyPluginsFromDir(importPath))
            return false;
    }

    // Process qtPluginsDirectory second (the installed Qt prefix).  Files
    // already present in dest (copied from plugins-import-paths above) are
    // skipped by copyFileIfNewer; plugins that exist only in the installed
    // prefix are copied normally.
    if (options.qtPluginsDirectory.isEmpty())
        return true;

    if (!QDir(options.qtPluginsDirectory).exists()) {
        if (options.verbose) {
            fprintf(stdout, "Qt plugins directory not found, skipping: %s\n",
                    qPrintable(options.qtPluginsDirectory));
        }
        return true;
    }

    if (options.verbose)
        fprintf(stdout, "Copying all Qt plugins from %s\n", qPrintable(options.qtPluginsDirectory));

    return copyPluginsFromDir(options.qtPluginsDirectory);
}

// Copy user-supplied extra plugins listed in QT_HARMONYOS_EXTRA_PLUGINS into
// entry/libs/<arch>/<category>/. The category is derived from the parent
// directory of each plugin source path (e.g. .../imageformats/libfoo.so ->
// "imageformats/libfoo.so"); plugins without a usable parent directory name
// are deployed flat under entry/libs/<arch>/.
static bool copyExtraPlugins(const Options &options)
{
    if (options.extraPlugins.isEmpty())
        return true;

    if (options.verbose)
        fprintf(stdout, "Copying extra plugins\n");

    for (const QString &pluginPath : options.extraPlugins) {
        const QFileInfo pluginInfo(pluginPath);
        if (!pluginInfo.exists() || !pluginInfo.isFile()) {
            fprintf(stderr, "Extra plugin does not exist: %s\n", qPrintable(pluginPath));
            return false;
        }

        const QString category = pluginInfo.absoluteDir().dirName();
        QString relativeDestPath;
        if (category.isEmpty() || category == "plugins"_L1)
            relativeDestPath = pluginInfo.fileName();
        else
            relativeDestPath = category + "/"_L1 + pluginInfo.fileName();

        if (options.verbose) {
            fprintf(stdout, "  Extra plugin: %s -> entry/libs/<arch>/%s\n",
                    qPrintable(pluginInfo.fileName()), qPrintable(relativeDestPath));
        }

        if (!copyFileToArchitectures(options, pluginInfo.filePath(), relativeDestPath))
            return false;
    }

    return true;
}

// .so files in the QML directory go flat to entry/libs/<arch>/; all other files
// preserve directory structure under resfile/qml/.
static bool copyQmlDir(const QString &srcDir, const QString &relPath,
                       const QString &qmlDestBase, const Options &options)
{
    const QFileInfoList entries =
        QDir(srcDir).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);

    for (const QFileInfo &entry : entries) {
        const QString entryRelPath = relPath.isEmpty()
            ? entry.fileName()
            : relPath + "/"_L1 + entry.fileName();

        if (entry.isDir()) {
            if (!copyQmlDir(entry.filePath(), entryRelPath, qmlDestBase, options))
                return false;
        } else if (entry.suffix() == "so"_L1) {
            for (const QString &arch : options.targetArchs) {
                QString destPath = options.outputDirectory + "/entry/libs/"_L1
                                  + arch + "/"_L1 + entry.fileName();
                QDir().mkpath(QFileInfo(destPath).absolutePath());
                if (!copyFileIfNewer(entry.filePath(), destPath, options.verbose))
                    return false;
            }
            if (!options.depFilePath.isEmpty())
                dependenciesForDepfile << entry.filePath();
        } else {
            QString destPath = qmlDestBase + "/"_L1 + entryRelPath;
            QDir().mkpath(QFileInfo(destPath).absolutePath());
            if (!copyFileIfNewer(entry.filePath(), destPath, options.verbose))
                return false;
            if (!options.depFilePath.isEmpty())
                dependenciesForDepfile << entry.filePath();
        }
    }
    return true;
}

static QString hapQmlDir(const Options &options)
{
    return options.outputDirectory + "/entry/src/main/resources/resfile/qml"_L1;
}

static bool copyAllQmlModules(const Options &options)
{
    const QString qmlDestBase = hapQmlDir(options);

    // Process qml-import-paths first (typically CMAKE_BINARY_DIR/qml, i.e. the
    // module's own build-tree QML output).  Files written here receive dest
    // mtime = now, so the subsequent qtQmlDirectory pass skips any file that
    // was already copied — this gives build-dir contents unconditional priority
    // over the installed Qt prefix without requiring a force-overwrite flag.
    for (const QString &importPath : options.qmlImportPaths) {
        if (!QDir(importPath).exists()) {
            if (options.verbose)
                fprintf(stdout, "QML import path not found, skipping: %s\n",
                        qPrintable(importPath));
            continue;
        }
        if (options.verbose)
            fprintf(stdout, "Copying QML modules from import path: %s\n",
                    qPrintable(importPath));
        if (!copyQmlDir(importPath, QString(), qmlDestBase, options))
            return false;
    }

    // Process qtQmlDirectory second (the installed Qt prefix).  Files that are
    // already present in dest (copied from qml-import-paths above) are skipped
    // by copyFileIfNewer; files that exist only here — e.g. QtQml/QtQuick when
    // building qtlottie against an installed qtdeclarative — are copied normally.
    if (options.qtQmlDirectory.isEmpty())
        return true;
    if (!QDir(options.qtQmlDirectory).exists()) {
        if (options.verbose)
            fprintf(stdout, "QML directory not found, skipping: %s\n",
                    qPrintable(options.qtQmlDirectory));
        return true;
    }
    if (options.verbose)
        fprintf(stdout, "Copying all QML modules from %s\n",
                qPrintable(options.qtQmlDirectory));
    return copyQmlDir(options.qtQmlDirectory, QString(), qmlDestBase, options);
}

static bool readQmldirLines(const QString &qmldirPath, QStringList &lines)
{
    QFile qmldir(qmldirPath);
    if (!qmldir.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "Failed to open qmldir %s: %s\n",
                qPrintable(qmldirPath), qPrintable(qmldir.errorString()));
        return false;
    }

    lines = QString::fromUtf8(qmldir.readAll()).split(u'\n');

    return true;
}

// Deploy the plugins (*.so files) a qmldir declares to libs/<arch>.
static bool copyQmldirPlugins(const QString &qmldirPath, const Options &options)
{
    QStringList lines;
    if (!readQmldirLines(qmldirPath, lines))
        return false;

    static const QRegularExpression pluginLine(
        "^\\s*(?:optional\\s+)?plugin\\s+(?<name>\\S+)(?:\\s+(?<path>\\S+))?"_L1);

    const QDir moduleDir = QFileInfo(qmldirPath).absoluteDir();

    for (const QString &line : lines) {
        const QRegularExpressionMatch match = pluginLine.match(line);
        if (!match.hasMatch())
            continue;

        const QString pluginName = match.captured(u"name");
        const QString pluginFile = "lib"_L1 + pluginName + ".so"_L1;
        const QString optionalPluginPath = match.captured(u"path");
        const QString pluginSrc = moduleDir.filePath(
            optionalPluginPath.isEmpty() ? pluginFile : optionalPluginPath + "/"_L1 + pluginFile);

        if (!QFileInfo::exists(pluginSrc)) {
            if (options.verbose) {
                fprintf(stdout, "  qmldir %s declares plugin %s, but %s is missing\n",
                        qPrintable(qmldirPath), qPrintable(pluginName),
                        qPrintable(pluginSrc));
            }
            continue;
        }

        if (!copyFileToArchitectures(options, pluginSrc, pluginFile))
            return false;
    }

    return true;
}

static bool getQmldirModuleUri(const QString &qmldirPath, QString &uri)
{
    QStringList lines;
    if (!readQmldirLines(qmldirPath, lines))
        return false;

    static const QRegularExpression moduleLine("^\\s*module\\s+(?<uri>\\S+)"_L1);
    for (const QString &line : lines) {
        const QRegularExpressionMatch match = moduleLine.match(line);
        if (match.hasMatch()) {
            uri = match.captured(u"uri");
            return true;
        }
    }

    uri.clear();

    return true;
}

static QString findNearestEnclosingTestDir(const QString &startDir, const QSet<QString> &testDirs)
{
    for (QDir dir(startDir); ; ) {
        const QString path = dir.absolutePath();

        if (testDirs.contains(path))
            return path;

        if (!dir.cdUp())
            return QString();
    }
}

struct TestQmlModule
{
    QString qmldirPath;
    QString testDir;
};

static bool getTestQmlModuleDeployDir(const TestQmlModule &testModule, QString &deployDir)
{
    QString uri;
    if (!getQmldirModuleUri(testModule.qmldirPath, uri))
        return false;

    if (uri.isEmpty()) {
        deployDir = QDir(testModule.testDir).relativeFilePath(
            QFileInfo(testModule.qmldirPath).absolutePath());
    } else {
        deployDir = uri.replace(u'.', u'/');
    }

    return true;
}

static QList<TestQmlModule> findTestQmlModules(const Options &options)
{
    if (options.testBinariesDirectory.isEmpty())
        return {};

    QStringList testBinaries;
    QStringList unusedHelperLibs;
    QSet<QString> unusedHelperLibNames;
    const QStringList excludeDirs = { QFileInfo(options.outputDirectory).absoluteFilePath() };
    scanTestBinariesDir(options.testBinariesDirectory, options.testExcludeList, excludeDirs,
                        testBinaries, unusedHelperLibs, unusedHelperLibNames);

    QSet<QString> testDirs;
    for (const QString &testBinary : std::as_const(testBinaries))
        testDirs.insert(QFileInfo(testBinary).absoluteDir().absolutePath());

    QList<TestQmlModule> modules;
    QSet<QString> seenQmldirs;
    for (const QString &testDir : std::as_const(testDirs)) {
        QDirIterator it(testDir, { "qmldir"_L1 }, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString qmldirPath = it.next();
            if (seenQmldirs.contains(qmldirPath))
                continue;

            const QString testDir =
                findNearestEnclosingTestDir(QFileInfo(qmldirPath).absolutePath(), testDirs);
            seenQmldirs.insert(qmldirPath);
            modules.append({ qmldirPath, testDir });
        }
    }

    return modules;
}

// Fail if two test QML modules in the bundle deploy to the same directory under
// resfile/qml, where one would overwrite the other.
static bool verifyUniqueTestQmlModuleDeployDirs(const QList<TestQmlModule> &modules)
{
    QHash<QString, QString> deployDirToQmldir;
    for (const TestQmlModule &module : modules) {
        QString deployDir;
        if (!getTestQmlModuleDeployDir(module, deployDir))
            return false;

        const auto existing = deployDirToQmldir.constFind(deployDir);
        if (existing != deployDirToQmldir.constEnd()) {
            fprintf(stderr,
                    "Error: two test QML modules deploy to the same directory \"%s\" "
                    "under resfile/qml:\n  %s\n  %s\n"
                    "They would overwrite one another; give one a different module URI.\n",
                    qPrintable(deployDir), qPrintable(*existing), qPrintable(module.qmldirPath));
            return false;
        }
        deployDirToQmldir.insert(deployDir, module.qmldirPath);
    }

    return true;
}

static bool copyTestQmlModuleFiles(const QString &moduleSrcDirPath,
                                   const QString &destModuleDirPath, const Options &options)
{
    static const QStringList qmlModuleFileNameFilters = {
        "qmldir"_L1,
        "*.qmltypes"_L1,
        "*.qml"_L1,
        "*.js"_L1,
        "*.mjs"_L1,
    };

    const QDir moduleSrcDir(moduleSrcDirPath);
    QDirIterator it(
        moduleSrcDirPath, qmlModuleFileNameFilters, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString srcPath = it.next();
        const QString destPath =
            destModuleDirPath + "/"_L1 + moduleSrcDir.relativeFilePath(srcPath);

        QDir().mkpath(QFileInfo(destPath).absolutePath());

        if (!copyFileIfNewer(srcPath, destPath, options.verbose))
            return false;

        if (!options.depFilePath.isEmpty())
            dependenciesForDepfile << srcPath;

        if (it.fileName() == "qmldir"_L1 && !copyQmldirPlugins(srcPath, options))
            return false;
    }

    return true;
}

// Deploy each test's generated QML modules into resfile/qml
static bool copyTestQmlModules(const QList<TestQmlModule> &modules, const Options &options)
{
    for (const TestQmlModule &module : modules) {
        const QString moduleSrcDir = QFileInfo(module.qmldirPath).absolutePath();

        QString deployDir;
        if (!getTestQmlModuleDeployDir(module, deployDir))
            return false;

        const QString destModuleDir = hapQmlDir(options) + "/"_L1 + deployDir;
        if (!copyTestQmlModuleFiles(moduleSrcDir, destModuleDir, options))
            return false;
    }

    return true;
}

static QString findLlvmReadobj(const Options &options)
{
    // Look for llvm-readobj in the NDK and alternative path
    const QStringList searchPaths = {
        options.ndkRoot + "/llvm/bin"_L1,
        options.sdkRoot + "/command-line-tools/sdk/default/openharmony/native/llvm/bin"_L1
    };

    const QString llvmReadobj = QStandardPaths::findExecutable("llvm-readobj"_L1, searchPaths);
    if (!llvmReadobj.isEmpty())
        return llvmReadobj;

    return QString();
}

struct QtDependency
{
    QString relativePath;  // e.g., "lib/libQt6Core.so"
    QString absolutePath;  // Full path on filesystem
};

// Returns the SONAME embedded in the ELF dynamic section of the library, or an empty
// string if it cannot be determined. The SONAME may differ from the on-disk filename
// (e.g. libicudata.so has SONAME libicudata.so.78).
static QString readElfSoname(const Options &options, const QString &binaryPath)
{
    const QString llvmReadobj = findLlvmReadobj(options);
    if (llvmReadobj.isEmpty())
        return QString();

    QProcess process;
    process.start(llvmReadobj, {"--dynamic"_L1, binaryPath});
    if (!process.waitForStarted() || !process.waitForFinished(30000))
        return QString();

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    // Output line: "  0x...E SONAME   Library soname: [libfoo.so.1]"
    for (const auto &line : output.split('\n'_L1)) {
        if (!line.contains("SONAME"_L1))
            continue;
        const int lb = line.indexOf('['_L1);
        const int rb = line.indexOf(']'_L1, lb);
        if (lb >= 0 && rb > lb)
            return line.mid(lb + 1, rb - lb - 1);
    }
    return QString();
}

static QStringList readElfDependencies(const Options &options, const QString &binaryPath)
{
    QString llvmReadobj = findLlvmReadobj(options);
    if (llvmReadobj.isEmpty()) {
        fprintf(stderr, "Warning: llvm-readobj not found, cannot detect dependencies\n");
        return QStringList();
    }

    QProcess process;
    QStringList arguments;
    arguments << "--needed-libs"_L1 << binaryPath;

    process.start(llvmReadobj, arguments);
    if (!process.waitForStarted()) {
        fprintf(stderr, "Failed to start llvm-readobj\n");
        return QStringList();
    }

    if (!process.waitForFinished(30000)) { // 30 second timeout
        fprintf(stderr, "llvm-readobj timed out\n");
        process.kill();
        return QStringList();
    }

    if (process.exitCode() != 0) {
        fprintf(stderr, "llvm-readobj failed with exit code %d\n", process.exitCode());
        return QStringList();
    }

    QStringList dependencies;
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QStringList lines = output.split('\n'_L1);

    bool inNeededLibs = false;
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("NeededLibraries"_L1)) {
            inNeededLibs = true;
            continue;
        }

        if (!inNeededLibs)
            continue;

        // Stop at next section
        if (trimmed.isEmpty() || trimmed.contains(':'_L1))
            break;

        // Extract library name
        if (trimmed.startsWith("lib"_L1))
            dependencies.append(trimmed);
    }

    return dependencies;
}

static QString findExtraDepLibrary(const Options &options, const QString &libName)
{
    for (const QString &dir : options.extraLibsDirs) {
        // Try exact name first (e.g. libicudata.so.78)
        QString libPath = dir + "/"_L1 + libName;
        if (QFile::exists(libPath))
            return libPath;
        // Fall back to unversioned name (e.g. libicudata.so) for libraries whose
        // SONAME carries a version suffix but the file on disk does not.
        int soIdx = libName.indexOf(".so."_L1);
        if (soIdx >= 0) {
            QString baseName = libName.left(soIdx + 3); // up to and including ".so"
            libPath = dir + "/"_L1 + baseName;
            if (QFile::exists(libPath))
                return libPath;
        }
    }
    return QString();
}

static bool isSystemLibrary(const QString &libName)
{
    // System libraries that should not be bundled
    return libName.startsWith("libc."_L1) ||
           libName.startsWith("libm."_L1) ||
           libName.startsWith("libdl."_L1) ||
           libName == "libEGL.so"_L1 ||
           libName == "libGLESv2.so"_L1 ||
           libName == "libGLESv3.so"_L1 ||
           libName.startsWith("libz."_L1) ||
           libName == "libc++_shared.so"_L1;
}

static QString findQtLibrary(const Options &options, const QString &libName)
{
    // Use qtLibsDirectory if provided (preferred method, from JSON config)
    if (!options.qtLibsDirectory.isEmpty()) {
        QString libPath = options.qtLibsDirectory + "/"_L1 + libName;
        if (QFile::exists(libPath))
            return libPath;
    }

    // Fallback: walk up from application binary to find Qt installation
    QFileInfo appInfo(options.applicationBinary);
    QDir dir(appInfo.absolutePath());
    for (int i = 0; i < 10; ++i) {
        // Check qtbase/lib (common for non-prefix builds)
        QString qtbaseLib = dir.absoluteFilePath("qtbase/lib"_L1);
        if (QDir(qtbaseLib).exists()) {
            QString libPath = qtbaseLib + "/"_L1 + libName;
            if (QFile::exists(libPath))
                return libPath;
        }
        // Check lib directory
        QString libDir = dir.absoluteFilePath("lib"_L1);
        if (QDir(libDir).exists()) {
            QString libPath = libDir + "/"_L1 + libName;
            if (QFile::exists(libPath))
                return libPath;
        }
        if (!dir.cdUp())
            break;
    }

    return QString();
}

static bool detectAndCopyDependencies(const Options &options, QSet<QString> &processedLibs)
{
    if (options.verbose)
        fprintf(stdout, "Detecting Qt library dependencies\n");

    // Start with the application binary
    QStringList toProcess;
    toProcess.append(options.applicationBinary);

    // Add project libraries to the ELF dependency analysis queue so their Qt
    // dependencies are also transitively scanned and deployed.
    for (const QString &projectLib : options.projectLibraries)
        toProcess.append(projectLib);

    QList<QtDependency> qtDependencies;
    QStringList detectedQtModules; // Track detected Qt module names
    bool needsStdCpp = false;

    while (!toProcess.isEmpty()) {
        QString currentLib = toProcess.takeFirst();

        if (processedLibs.contains(currentLib))
            continue;

        processedLibs.insert(currentLib);

        if (options.verbose)
            fprintf(stdout, "  Analyzing: %s\n", qPrintable(QFileInfo(currentLib).fileName()));

        QStringList deps = readElfDependencies(options, currentLib);

        for (const QString &dep : deps) {
            // Check if we need C++ standard library
            if (dep == "libc++_shared.so"_L1) {
                needsStdCpp = true;
                continue;
            }

            // Skip system libraries
            if (isSystemLibrary(dep))
                continue;

            // Only process Qt libraries, platform plugins, and third-party libs from extra dirs
            if (!dep.startsWith("libQt6"_L1) && !dep.startsWith("libqohos"_L1)) {
                // Check extra library search directories (e.g. HARMONYOS_DEPS_ROOT/lib)
                QString extraDepPath = findExtraDepLibrary(options, dep);
                if (extraDepPath.isEmpty())
                    continue;
                // Guard against duplicates without blocking recursive ELF scanning:
                // do NOT insert into processedLibs here — the while-loop dequeue does
                // that, which also ensures the library's own ELF deps get scanned.
                if (!processedLibs.contains(extraDepPath) && !toProcess.contains(extraDepPath)) {
                    if (options.verbose)
                        fprintf(stdout, "    Found extra dep: %s\n", qPrintable(dep));
                    QtDependency extraDep;
                    extraDep.relativePath = "lib/"_L1 + dep;
                    extraDep.absolutePath = extraDepPath;
                    qtDependencies.append(extraDep);
                    toProcess.append(extraDepPath);
                }
                continue;
            }

            // Extract module name from Qt library (e.g., libQt6Core.so -> Core)
            if (dep.startsWith("libQt6"_L1)) {
                QString moduleName = dep.mid(6); // Skip "libQt6"
                if (moduleName.endsWith(".so"_L1))
                    moduleName.chop(3);
                if (!moduleName.isEmpty() && !detectedQtModules.contains(moduleName))
                    detectedQtModules.append(moduleName);
            }

            QString depPath = findQtLibrary(options, dep);
            if (depPath.isEmpty()) {
                if (options.verbose)
                    fprintf(stdout, "    Warning: Could not find Qt library: %s\n", qPrintable(dep));
                continue;
            }

            if (processedLibs.contains(depPath))
                continue;

            if (options.verbose)
                fprintf(stdout, "    Found dependency: %s\n", qPrintable(dep));

            QtDependency qtDep;
            qtDep.relativePath = "lib/"_L1 + dep;
            qtDep.absolutePath = depPath;
            qtDependencies.append(std::move(qtDep));

            // Add to processing queue for recursive dependency detection
            toProcess.append(std::move(depPath));
        }
    }

    if (options.verbose) {
        fprintf(stdout, "Found %lld Qt library dependencies\n", static_cast<long long>(qtDependencies.size()));
        if (needsStdCpp)
            fprintf(stdout, "C++ standard library required\n");
    }

    // Copy C++ standard library if needed (per-arch, since source path is arch-specific)
    if (needsStdCpp) {
        for (const QString &arch : options.targetArchs) {
            QString archLibPath = options.outputDirectory + "/entry/libs/"_L1 + arch;
            QString stdCppPath = findStdCppLibrary(options, arch);
            if (stdCppPath.isEmpty()) {
                fprintf(stderr, "Warning: Could not find C++ standard library for %s\n", qPrintable(arch));
            } else {
                QString destPath = archLibPath + "/libc++_shared.so"_L1;
                if (options.verbose)
                    fprintf(stdout, "  Copying C++ standard library for %s\n", qPrintable(arch));
                if (!copyFileIfNewer(stdCppPath, destPath, options.verbose)) {
                    fprintf(stderr,
                            "Failed to copy C++ standard library to: %s\n",
                            qPrintable(destPath));
                    return false;
                }

                // Track as dependency for depfile
                if (!options.depFilePath.isEmpty())
                    dependenciesForDepfile << stdCppPath;
            }
        }
    }

    // Copy all detected Qt/extra libraries to all target architectures.
    // copyFileToArchitectures handles all arches internally; call it once per library.
    // Use dep.relativePath filename so the deployed name matches the ELF SONAME
    // (e.g. libicudata.so.78, not the unversioned on-disk name libicudata.so).
    for (const QtDependency &dep : qtDependencies) {
        QFileInfo libInfo(dep.relativePath);
        if (!copyFileToArchitectures(options, dep.absolutePath, libInfo.fileName(), false))
            return false;

        if (!options.depFilePath.isEmpty())
            dependenciesForDepfile << dep.absolutePath;
    }

    return true;
}

static QString findQtPluginsDirectory(const Options &options)
{
    // 1. Use qtPluginsDirectory if provided (from JSON config)
    if (!options.qtPluginsDirectory.isEmpty() &&
        QDir(options.qtPluginsDirectory).exists()) {
        return options.qtPluginsDirectory;
    }

    // 2. Fallback: walk up from application binary
    QFileInfo appInfo(options.applicationBinary);
    QDir dir(appInfo.absolutePath());
    for (int i = 0; i < 10; ++i) {
        // Check qtbase/plugins for modular builds
        QString candidate = dir.absoluteFilePath("qtbase/plugins"_L1);
        if (QDir(candidate).exists())
            return candidate;

        // Check plugins directory
        if (dir.exists("plugins"_L1))
            return dir.absoluteFilePath("plugins"_L1);

        if (!dir.cdUp())
            break;
    }

    return QString();
}

static bool copyPlatformPlugin(const Options &options,
                               const QString &qtPluginsPath,
                               QSet<QString> &processedLibs)
{
    // Copy libqohos.so to ROOT libs directory (not in platforms subdirectory)
    QString qohosPlugin = qtPluginsPath + "/platforms/libqohos.so"_L1;
    if (!QFile::exists(qohosPlugin)) {
        fprintf(stderr, "Warning: Platform plugin libqohos.so not found at: %s\n",
               qPrintable(qohosPlugin));
        return true; // Not fatal
    }

    // Detect dependencies of libqohos.so
    if (options.verbose)
        fprintf(stdout, "  Detecting platform plugin dependencies\n");

    QStringList pluginDeps = readElfDependencies(options, qohosPlugin);
    QList<QtDependency> additionalLibs;

    for (const QString &dep : pluginDeps) {
        // Only process Qt libraries we haven't already copied
        if (!dep.startsWith("libQt6"_L1))
            continue;

        QString depPath = findQtLibrary(options, dep);
        if (depPath.isEmpty()) {
            if (options.verbose)
                fprintf(stdout, "    Warning: Could not find plugin dependency: %s\n",
                       qPrintable(dep));
            continue;
        }

        if (processedLibs.contains(depPath))
            continue;

        if (options.verbose)
            fprintf(stdout, "    Found plugin dependency: %s\n", qPrintable(dep));

        processedLibs.insert(depPath);
        QtDependency qtDep;
        qtDep.relativePath = "lib/"_L1 + dep;
        qtDep.absolutePath = std::move(depPath);
        additionalLibs.append(std::move(qtDep));
    }

    // Copy additional Qt libraries needed by the plugin
    for (const QString &arch : options.targetArchs) {
        QString archLibPath = options.outputDirectory + "/entry/libs/"_L1 + arch;

        for (const QtDependency &dep : additionalLibs) {
            QString destPath = archLibPath + "/"_L1 + QFileInfo(dep.absolutePath).fileName();

            if (options.verbose) {
                fprintf(stdout, "  Copying plugin dependency for %s: %s\n",
                       qPrintable(arch), qPrintable(QFileInfo(dep.absolutePath).fileName()));
            }

            if (!copyFileIfNewer(dep.absolutePath, destPath, options.verbose)) {
                fprintf(stderr, "Failed to copy library: %s\n", qPrintable(destPath));
                return false;
            }

            // Track as dependency for depfile
            if (!options.depFilePath.isEmpty())
                dependenciesForDepfile << dep.absolutePath;
        }
    }

    // Now copy libqohos.so itself to root
    for (const QString &arch : options.targetArchs) {
        QString archLibPath = options.outputDirectory + "/entry/libs/"_L1 + arch;
        QString destPath = archLibPath + "/libqohos.so"_L1;

        if (options.verbose)
            fprintf(stdout, "  Copying platform plugin for %s: libqohos.so (to root)\n",
                   qPrintable(arch));

        if (!copyFileIfNewer(qohosPlugin, destPath, options.verbose)) {
            fprintf(stderr, "Failed to copy libqohos.so to: %s\n", qPrintable(destPath));
            return false;
        }

        // Track as dependency for depfile
        if (!options.depFilePath.isEmpty())
            dependenciesForDepfile << qohosPlugin;
    }

    return true;
}

static bool copyPlugins(const Options &options, QSet<QString> &processedLibs)
{
    if (options.verbose)
        fprintf(stdout, "Copying required Qt plugins\n");

    // Find Qt plugins directory
    QString qtPluginsPath = findQtPluginsDirectory(options);
    if (qtPluginsPath.isEmpty()) {
        fprintf(stderr, "Warning: Could not find Qt plugins directory\n");
        return true; // Not fatal
    }

    if (options.verbose)
        fprintf(stdout, "  Qt plugins directory: %s\n", qPrintable(qtPluginsPath));

    // Copy platform plugin (special case: goes to root, not platforms/)
    if (!copyPlatformPlugin(options, qtPluginsPath, processedLibs))
        return false;

    // Discover and copy all other plugins based on dependencies
    QDir pluginsDir(qtPluginsPath);
    QStringList pluginCategories = pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (options.verbose)
        fprintf(stdout, "Scanning plugin categories: %s\n",
               qPrintable(pluginCategories.join(", "_L1)));

    for (const QString &category : pluginCategories) {
        // Skip platforms category - libqohos.so already handled specially above
        if (category == "platforms"_L1)
            continue;

        QDir categoryDir(pluginsDir.filePath(category));
        QStringList plugins = categoryDir.entryList({"*.so"_L1}, QDir::Files);

        if (plugins.isEmpty())
            continue;

        if (options.verbose)
            fprintf(stdout, "\nChecking %s plugins (%lld found):\n",
                   qPrintable(category), static_cast<long long>(plugins.size()));

        for (const QString &plugin : plugins) {
            QString pluginPath = categoryDir.filePath(plugin);

            // Read plugin's ELF dependencies
            QStringList deps = readElfDependencies(options, pluginPath);

            // Check if all Qt dependencies are satisfied
            bool allDepsSatisfied = true;
            QStringList unsatisfiedDeps;

            for (const QString &dep : deps) {
                // Skip system libraries
                if (isSystemLibrary(dep))
                    continue;

                // Check if Qt6 dependency is being included
                if (dep.startsWith("libQt6"_L1)) {
                    QString depPath = findQtLibrary(options, dep);
                    if (depPath.isEmpty() || !processedLibs.contains(depPath)) {
                        allDepsSatisfied = false;
                        unsatisfiedDeps.append(dep);
                    }
                }
            }

            if (allDepsSatisfied) {
                // Copy this plugin to entry/libs/{arch}/{category}/
                if (options.verbose)
                    fprintf(stdout, "  [✓] %s (dependencies satisfied)\n", qPrintable(plugin));

                QString relativeDestPath = "%1/%2"_L1.arg(category, plugin);
                if (!copyFileToArchitectures(options, pluginPath, relativeDestPath))
                    return false;
            } else {
                if (options.verbose) {
                    fprintf(stdout, "  [✗] %s (missing: %s)\n",
                           qPrintable(plugin), qPrintable(unsatisfiedDeps.join(", "_L1)));
                }
            }
        }
    }

    if (options.verbose)
        fprintf(stdout, "Plugin copying completed\n");

    return true;
}

struct QmlImportInfo
{
    QString name;                  // Module name (e.g., "QtQuick")
    QString path;                  // Absolute path to module directory
    QString type;                  // "module" or "plugin"
    QString plugin;                // Plugin name (e.g., "qtquick2plugin")
    bool pluginIsOptional = false; // Whether plugin is optional
    QString prefer; // Preferred location (e.g., ":/" means embedded in resources)
    QStringList components; // List of QML component file paths
    QStringList scripts;    // List of JavaScript file paths
};

static QList<QmlImportInfo> scanQmlImports(const Options &options)
{
    QList<QmlImportInfo> imports;

    if (options.qmlRootPaths.isEmpty()) {
        if (options.verbose)
            fprintf(stdout,
                    "No QML root path specified, skipping QML import scanning\n");
        return imports;
    }

    if (options.verbose)
        fprintf(stdout, "Scanning for QML imports\n");

    // 1. Use qtLibExecsDirectory if provided (preferred, from JSON config)
    QStringList searchPaths;
    if (!options.qtLibExecsDirectory.isEmpty())
        searchPaths.append(options.qtLibExecsDirectory);

    // 2. Try qtHostDirectory/bin as fallback
    if (!options.qtHostDirectory.isEmpty())
        searchPaths.append(options.qtHostDirectory + "/bin"_L1);

    QString qmlImportScannerPath =
        QStandardPaths::findExecutable("qmlimportscanner"_L1, searchPaths);

    // 3. Fallback: search from application binary path
    if (qmlImportScannerPath.isEmpty()) {
        QDir dir(QFileInfo(options.applicationBinary).absolutePath());

        for (int i = 0; i < 10; ++i) {
            qmlImportScannerPath = QStandardPaths::findExecutable(
                "qmlimportscanner"_L1,
                {dir.absoluteFilePath("libexec"_L1), dir.absoluteFilePath("bin"_L1)});

            if (!qmlImportScannerPath.isEmpty())
                break;

            if (!dir.cdUp())
                break;
        }
    }

    if (qmlImportScannerPath.isEmpty()) {
        fprintf(
            stderr,
            "Warning: qmlimportscanner not found, skipping QML import scanning\n");
        return imports;
    }

    if (options.verbose)
        fprintf(stdout, "  Using qmlimportscanner: %s\n",
                qPrintable(qmlImportScannerPath));

    // Build import paths argument
    QStringList importPaths;

    // Add QML import paths from config
    for (const QString &path : options.qmlImportPaths)
        if (QFile::exists(path))
            importPaths.append(path);

    // Add application build directory for locally-built QML modules
    // This is needed to find modules like "shared" that are built alongside the
    // app
    QFileInfo appBinary(options.applicationBinary);
    QString appBuildDir = appBinary.absolutePath();
    if (QDir(appBuildDir).exists())
        importPaths.append(appBuildDir);

    // Add Qt QML directory (target platform)
    if (!options.qtQmlDirectory.isEmpty() &&
        QDir(options.qtQmlDirectory).exists()) {
        importPaths.append(options.qtQmlDirectory);
    } else {
        // Fallback: search from application binary path
        QFileInfo appBinary(options.applicationBinary);
        QDir qtDir(appBinary.absolutePath());
        for (int i = 0; i < 10; ++i) {
            QString qmlDir = qtDir.absoluteFilePath("qml"_L1);
            if (QDir(qmlDir).exists()) {
                importPaths.append(qmlDir);
                break;
            }
            // Also check qtbase/../qml for modular builds
            qmlDir = qtDir.absoluteFilePath("qtbase/../qml"_L1);
            if (QDir(qmlDir).exists()) {
                importPaths.append(QDir::cleanPath(qmlDir));
                break;
            }
            if (!qtDir.cdUp())
                break;
        }
    }

    if (importPaths.isEmpty()) {
        fprintf(stderr, "Warning: No QML import paths found\n");
        return imports;
    }

    // Build qmlimportscanner command. qmlimportscanner accepts a -rootPath flag
    // per root, so emit them in order.
    QStringList arguments;
    for (const QString &rootPath : options.qmlRootPaths)
        arguments << "-rootPath"_L1 << rootPath;

    for (const QString &importPath : importPaths)
        arguments << "-importPath"_L1 << importPath;

    if (options.verbose) {
        fprintf(stdout, "  Root paths:\n");
        for (const QString &rootPath : options.qmlRootPaths)
            fprintf(stdout, "    %s\n", qPrintable(rootPath));
        fprintf(stdout, "  Import paths:\n");
        for (const QString &path : importPaths)
            fprintf(stdout, "    %s\n", qPrintable(path));
    }

    // Run qmlimportscanner
    QProcess process;
    process.start(qmlImportScannerPath, arguments);

    if (!process.waitForFinished(30000)) {
        fprintf(stderr, "Error: qmlimportscanner timed out\n");
        return imports;
    }

    if (process.exitCode() != 0) {
        fprintf(stderr, "Error: qmlimportscanner failed with exit code %d\n",
                process.exitCode());
        fprintf(stderr, "%s\n", process.readAllStandardError().constData());
        return imports;
    }

    // Parse JSON output
    QByteArray output = process.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output);

    if (!doc.isArray()) {
        fprintf(stderr, "Error: Invalid JSON output from qmlimportscanner\n");
        return imports;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;

        QJsonObject obj = value.toObject();
        QmlImportInfo info;
        info.name = obj["name"_L1].toString();
        info.path = obj["path"_L1].toString();
        info.type = obj["type"_L1].toString();

        if (obj.contains("plugin"_L1))
            info.plugin = obj["plugin"_L1].toString();

        if (obj.contains("pluginIsOptional"_L1))
            info.pluginIsOptional = obj["pluginIsOptional"_L1].toBool();

        if (obj.contains("prefer"_L1))
            info.prefer = obj["prefer"_L1].toString();

        // Parse components array
        if (obj.contains("components"_L1)) {
            QJsonArray componentsArray = obj["components"_L1].toArray();
            for (const QJsonValue &comp : componentsArray)
                info.components.append(comp.toString());
        }

        // Parse scripts array
        if (obj.contains("scripts"_L1)) {
            QJsonArray scriptsArray = obj["scripts"_L1].toArray();
            for (const QJsonValue &script : scriptsArray)
                info.scripts.append(script.toString());
        }

        // Skip if path is empty (unresolved import)
        if (info.path.isEmpty()) {
            if (options.verbose)
                fprintf(stdout, "  Warning: Could not resolve QML import: %s\n",
                        qPrintable(info.name));
            continue;
        }

        // Skip if type is not module
        if (info.type != "module"_L1)
            continue;

        if (options.verbose)
            fprintf(stdout, "  Found QML import: %s at %s\n", qPrintable(info.name),
                    qPrintable(info.path));

        imports.append(info);
    }

    return imports;
}

static bool copyQmlFiles(const Options &options)
{
    if (options.qmlRootPaths.isEmpty())
        return true; // Not an error, just no QML files to copy

    // Target directory: entry/src/main/resources/rawfile/qml/.
    //
    // Multiple roots are merged into the same destination. copyFileIfNewer
    // only overwrites when the source mtime is newer than the destination,
    // so in practice the *first* root to land a given file wins. User-set
    // QT_QML_ROOT_PATH values are emitted before auto-collected roots (see
    // _qt_internal_harmonyos_format_deployment_paths in Qt6HarmonyOSMacros.cmake),
    // giving explicit user settings precedence over auto-collected ones on
    // collision.
    const QString destDir =
        options.outputDirectory + "/entry/src/main/resources/rawfile/qml"_L1;
    if (!QDir().mkpath(destDir)) {
        fprintf(stderr, "Failed to create QML destination directory: %s\n",
                qPrintable(destDir));
        return false;
    }

    for (const QString &rootPath : options.qmlRootPaths) {
        if (options.verbose)
            fprintf(stdout, "Copying QML files from %s\n", qPrintable(rootPath));

        if (!copyRecursively(rootPath, destDir, options.verbose)) {
            fprintf(stderr, "Failed to copy QML files from %s\n", qPrintable(rootPath));
            return false;
        }
    }

    if (options.verbose)
        fprintf(stdout, "QML files copied successfully\n");

    return true;
}

static bool copyQmlImports(const Options &options,
                           const QList<QmlImportInfo> &imports,
                           QSet<QString> &processedLibs)
{
    if (imports.isEmpty())
        return true;

    if (options.verbose)
        fprintf(stdout, "Copying QML imports\n");

    // QML non-.so files go to resfile/qml/ (maintaining directory structure)
    // QML plugin .so files go to libs/arm64-v8a/ (flat, no subdirectory)
    QString qmlDestBase = hapQmlDir(options);
    QDir().mkpath(qmlDestBase);

    // Track QML plugins to scan for dependencies
    QStringList qmlPluginsToScan;

    for (const QmlImportInfo &import : imports) {
        if (options.verbose)
            fprintf(stdout, "  Copying QML module: %s\n", qPrintable(import.name));

        // Determine module subdirectory - preserve full path structure
        // e.g., QtQuick.Window should go to QtQuick/Window/, not just Window/
        QString relativePath;

        // Calculate relative path from Qt QML directory
        if (!options.qtQmlDirectory.isEmpty() && import.path.startsWith(options.qtQmlDirectory)) {
            // Qt module - use relative path from Qt QML dir
            relativePath = import.path.mid(options.qtQmlDirectory.length());
            if (relativePath.startsWith('/'_L1))
                relativePath = relativePath.mid(1);
        } else {
            // Application module or other - use module name converted to path
            // e.g., "QtQuick.Window" -> "QtQuick/Window"
            relativePath = import.name;
            relativePath.replace('.'_L1, '/'_L1);
        }

        QString destModuleDir = qmlDestBase + "/"_L1 + relativePath;
        QDir().mkpath(destModuleDir);

        // Copy qmldir file (required for module discovery)
        QString qmldirSrc = import.path + "/qmldir"_L1;
        QString qmldirDest = destModuleDir + "/qmldir"_L1;
        if (QFile::exists(qmldirSrc)) {
            if (copyFileIfNewer(qmldirSrc, qmldirDest, options.verbose)) {
                if (options.verbose)
                    fprintf(stdout, "    Copied qmldir\n");

                // Track as dependency for depfile
                if (!options.depFilePath.isEmpty())
                    dependenciesForDepfile << qmldirSrc;
            } else {
                fprintf(stderr, "Warning: Failed to copy qmldir for %s\n",
                        qPrintable(import.name));
            }
        }

        // Copy plugin library if not embedded in resources
        // Check "prefer" field - if it starts with ":/" then QML files are in
        // resources
        bool qmlFilesAreEmbedded = import.prefer.startsWith(":/"_L1);

        if (!import.plugin.isEmpty()) {
            // QML plugin .so files go to libs/arm64-v8a/ (flat, per HarmonyOS requirements)
            QString pluginFileName = "lib"_L1 + import.plugin + ".so"_L1;
            QString pluginSrc = import.path + "/"_L1 + pluginFileName;

            if (QFile::exists(pluginSrc)) {
                // Copy to libs directory for each architecture
                for (const QString &arch : options.targetArchs) {
                    QString pluginDest = options.outputDirectory + "/entry/libs/"_L1 + arch
                                        + "/"_L1 + pluginFileName;

                    if (copyFileIfNewer(pluginSrc, pluginDest, options.verbose)) {
                        if (options.verbose)
                            fprintf(stdout, "    Copied plugin to libs/%s: %s\n",
                                    qPrintable(arch), qPrintable(pluginFileName));
                        processedLibs.insert(pluginSrc);

                        // Add plugin to list for dependency scanning
                        if (!qmlPluginsToScan.contains(pluginSrc))
                            qmlPluginsToScan.append(pluginSrc);

                        // Track as dependency for depfile
                        if (!options.depFilePath.isEmpty())
                            dependenciesForDepfile << pluginSrc;
                    } else if (!import.pluginIsOptional) {
                        fprintf(stderr, "Warning: Failed to copy required plugin: %s\n",
                                qPrintable(pluginFileName));
                    }
                }
            } else if (!import.pluginIsOptional) {
                if (options.verbose)
                    fprintf(stdout, "    Warning: Required plugin not found: %s\n",
                            qPrintable(pluginFileName));
            }
        }

        // Copy QML component files (only if not embedded in resources)
        if (!qmlFilesAreEmbedded) {
            for (const QString &component : import.components) {
                QFileInfo compInfo(component);
                if (!compInfo.exists()) {
                    if (options.verbose)
                        fprintf(stdout, "    Warning: Component file not found: %s\n",
                                qPrintable(component));
                    continue;
                }

                QString relativePath = component.mid(import.path.length());
                if (relativePath.startsWith('/'_L1))
                    relativePath = relativePath.mid(1);

                QString destFile = destModuleDir + "/"_L1 + relativePath;
                QFileInfo destInfo(destFile);
                QDir().mkpath(destInfo.absolutePath());

                if (copyFileIfNewer(component, destFile, options.verbose)) {
                    if (options.verbose)
                        fprintf(stdout, "    Copied component: %s\n", qPrintable(compInfo.fileName()));

                    // Track as dependency for depfile
                    if (!options.depFilePath.isEmpty())
                        dependenciesForDepfile << component;
                }
            }

            // Copy JavaScript files
            for (const QString &script : import.scripts) {
                QFileInfo scriptInfo(script);
                if (!scriptInfo.exists())
                    continue;

                QString relativePath = script.mid(import.path.length());
                if (relativePath.startsWith('/'_L1))
                    relativePath = relativePath.mid(1);

                QString destFile = destModuleDir + "/"_L1 + relativePath;
                QFileInfo destInfo(destFile);
                QDir().mkpath(destInfo.absolutePath());

                if (copyFileIfNewer(script, destFile, options.verbose)) {
                    if (options.verbose)
                        fprintf(stdout, "    Copied script: %s\n", qPrintable(scriptInfo.fileName()));

                    // Track as dependency for depfile
                    if (!options.depFilePath.isEmpty())
                        dependenciesForDepfile << script;
                }
            }
        } else {
            if (options.verbose)
                fprintf(stdout, "    Skipping QML files (embedded in resources)\n");
        }
    }

    // Scan QML plugin dependencies and copy any missing Qt libraries
    if (!qmlPluginsToScan.isEmpty()) {
        if (options.verbose)
            fprintf(stdout, "Scanning QML plugin dependencies\n");

        QStringList toProcess = qmlPluginsToScan;
        while (!toProcess.isEmpty()) {
            QString pluginPath = toProcess.takeFirst();

            if (options.verbose)
                fprintf(stdout, "  Scanning plugin: %s\n", qPrintable(QFileInfo(pluginPath).fileName()));

            QStringList deps = readElfDependencies(options, pluginPath);
            for (const QString &dep : deps) {
                // Skip system libraries
                if (isSystemLibrary(dep))
                    continue;

                // Resolve the library path from Qt libs or extra-libs-dirs
                QString depPath;
                if (dep.startsWith("libQt6"_L1)) {
                    depPath = findQtLibrary(options, dep);
                    if (depPath.isEmpty()) {
                        if (options.verbose)
                            fprintf(stdout, "    Warning: Could not find Qt library: %s\n", qPrintable(dep));
                        continue;
                    }
                } else if (!options.extraLibsDirs.isEmpty()) {
                    depPath = findExtraDepLibrary(options, dep);
                    if (depPath.isEmpty())
                        continue;
                } else {
                    continue;
                }

                if (processedLibs.contains(depPath))
                    continue;

                if (options.verbose)
                    fprintf(stdout, "    Found dependency: %s\n", qPrintable(dep));

                // Copy the library
                for (const QString &arch : options.targetArchs) {
                    QString destPath = options.outputDirectory + "/entry/libs/"_L1 + arch + "/"_L1 + dep;
                    if (copyFileIfNewer(depPath, destPath, options.verbose)) {
                        if (options.verbose)
                            fprintf(stdout, "    Copied %s to libs/%s\n", qPrintable(dep), qPrintable(arch));

                        // Track as dependency for depfile
                        if (!options.depFilePath.isEmpty())
                            dependenciesForDepfile << depPath;
                    }
                }

                processedLibs.insert(depPath);
                // Recursively scan this dependency
                toProcess.append(depPath);
            }
        }
    }

    if (options.verbose)
        fprintf(stdout, "QML imports copied successfully\n");

    return true;
}

// build-profile.json5 ships with `"signingConfigs": []`; replace that literal
// with a populated block when the user supplies signing material. JSON5 input
// (trailing commas, // comments) precludes QJsonDocument, hence string surgery.
static bool injectSigningConfig(const Options &options)
{
    struct Field
    {
        const char *envName;
        const char *cliFlag;
        QString cliValue;
        QByteArray envValue;

        QByteArray resolved() const
        {
            if (!cliValue.isEmpty())
                return cliValue.toUtf8();
            return envValue;
        }
        QString sourceLabel() const
        {
            return cliValue.isEmpty()
                ? QString::fromLatin1(envName)
                : QString::fromLatin1(cliFlag);
        }
    };

    Field required[] = {
        { "QT_HARMONYOS_SIGNING_CERT_PATH",      "--signing-cert-path",
          options.signingCertPath,      qgetenv("QT_HARMONYOS_SIGNING_CERT_PATH")      },
        { "QT_HARMONYOS_SIGNING_PROFILE",        "--signing-profile",
          options.signingProfile,       qgetenv("QT_HARMONYOS_SIGNING_PROFILE")        },
        { "QT_HARMONYOS_SIGNING_STORE_FILE",     "--signing-store-file",
          options.signingStoreFile,     qgetenv("QT_HARMONYOS_SIGNING_STORE_FILE")     },
        { "QT_HARMONYOS_SIGNING_KEY_ALIAS",      "--signing-key-alias",
          options.signingKeyAlias,      qgetenv("QT_HARMONYOS_SIGNING_KEY_ALIAS")      },
        { "QT_HARMONYOS_SIGNING_KEY_PASSWORD",   "--signing-key-password",
          options.signingKeyPassword,   qgetenv("QT_HARMONYOS_SIGNING_KEY_PASSWORD")   },
        { "QT_HARMONYOS_SIGNING_STORE_PASSWORD", "--signing-store-password",
          options.signingStorePassword, qgetenv("QT_HARMONYOS_SIGNING_STORE_PASSWORD") },
    };

    QByteArray signAlg = options.signingAlg.isEmpty()
        ? qgetenv("QT_HARMONYOS_SIGNING_ALG")
        : options.signingAlg.toUtf8();

    bool anySet = !signAlg.isEmpty();
    for (const Field &f : required)
        anySet = anySet || !f.resolved().isEmpty();
    if (!anySet)
        return true;

    QStringList missing;
    for (const Field &f : required) {
        if (f.resolved().isEmpty())
            missing << QString::fromLatin1(f.cliFlag) + " / "_L1
                       + QString::fromLatin1(f.envName);
    }
    if (!missing.isEmpty()) {
        fprintf(stderr, "Error: HAP signing requested, but the following required input(s)\n"
                        "       are missing (neither CLI flag nor env var was supplied):\n");
        for (const QString &name : missing)
            fprintf(stderr, "         %s\n", qPrintable(name));
        return false;
    }

    if (signAlg.isEmpty())
        signAlg = "SHA256withECDSA";

    const QString buildProfilePath = options.outputDirectory + "/build-profile.json5"_L1;
    QFile profileFile(buildProfilePath);
    if (!profileFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "Error: cannot open %s for reading: %s\n",
                qPrintable(buildProfilePath), qPrintable(profileFile.errorString()));
        return false;
    }
    QByteArray content = profileFile.readAll();
    profileFile.close();

    // Anything other than the literal empty array means the user already edited
    // the file; refuse rather than risk corrupting it.
    static const QByteArray needle = "\"signingConfigs\": []";
    const int idx = content.indexOf(needle);
    if (idx < 0) {
        fprintf(stderr, "Error: '%s' not found in %s. The template may have been modified;\n"
                        "       cannot inject signing configuration safely.\n",
                needle.constData(), qPrintable(buildProfilePath));
        return false;
    }

    // Only backslash and double-quote need escaping; inputs are paths, aliases,
    // and hex blobs — no control characters.
    auto escapeJsonString = [](const QByteArray &in) {
        QByteArray out;
        out.reserve(in.size());
        for (char c : in) {
            if (c == '\\' || c == '"')
                out.append('\\');
            out.append(c);
        }
        return out;
    };

    QByteArray replacement;
    replacement.append("\"signingConfigs\": [\n");
    replacement.append("      {\n");
    replacement.append("        \"name\": \"default\",\n");
    replacement.append("        \"type\": \"HarmonyOS\",\n");
    replacement.append("        \"material\": {\n");
    auto appendField = [&](const char *key, const QByteArray &value, bool last) {
        replacement.append("          \"");
        replacement.append(key);
        replacement.append("\": \"");
        replacement.append(escapeJsonString(value));
        replacement.append('"');
        if (!last)
            replacement.append(',');
        replacement.append('\n');
    };
    appendField("certpath",      required[0].resolved(), false);
    appendField("keyAlias",      required[3].resolved(), false);
    appendField("keyPassword",   required[4].resolved(), false);
    appendField("profile",       required[1].resolved(), false);
    appendField("signAlg",       signAlg,                false);
    appendField("storeFile",     required[2].resolved(), false);
    appendField("storePassword", required[5].resolved(), true);
    replacement.append("        }\n");
    replacement.append("      }\n");
    replacement.append("    ]");

    content.replace(idx, needle.size(), replacement);

    if (!profileFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        fprintf(stderr, "Error: cannot open %s for writing: %s\n",
                qPrintable(buildProfilePath), qPrintable(profileFile.errorString()));
        return false;
    }
    if (profileFile.write(content) != content.size()) {
        fprintf(stderr, "Error: failed to write full content to %s\n",
                qPrintable(buildProfilePath));
        profileFile.close();
        return false;
    }
    profileFile.close();

    if (options.verbose) {
        fprintf(stdout, "Injected HAP signingConfig into %s\n", qPrintable(buildProfilePath));
        fprintf(stdout, "  certpath:  %s  (from %s)\n",
                required[0].resolved().constData(), qPrintable(required[0].sourceLabel()));
        fprintf(stdout, "  profile:   %s  (from %s)\n",
                required[1].resolved().constData(), qPrintable(required[1].sourceLabel()));
        fprintf(stdout, "  storeFile: %s  (from %s)\n",
                required[2].resolved().constData(), qPrintable(required[2].sourceLabel()));
        fprintf(stdout, "  keyAlias:  %s  (from %s)\n",
                required[3].resolved().constData(), qPrintable(required[3].sourceLabel()));
        fprintf(stdout, "  signAlg:   %s\n", signAlg.constData());
    }
    return true;
}

static bool buildHap(const Options &options, QString *hapOutputPath = nullptr)
{
    if (!options.buildPackage) {
        if (options.verbose)
            fprintf(stdout, "Skipping HAP build (--no-build specified)\n");
        return true;
    }

    // Resolve hvigorw path: CLI option → env var → auto-detect in output directory
    QString hvigorPath = options.hvigorPath;

    if (hvigorPath.isEmpty()) {
        QByteArray envHvigor = qgetenv("QT_HARMONYOS_HVIGOR");
        if (!envHvigor.isEmpty())
            hvigorPath = QString::fromLocal8Bit(envHvigor);
    }

    if (hvigorPath.isEmpty()) {
        // Auto-detect hvigorw in the output directory (always present in the template)
        QString candidate = options.outputDirectory + "/hvigorw"_L1;
        if (QFile::exists(candidate))
            hvigorPath = std::move(candidate);
    }

    if (hvigorPath.isEmpty()) {
        fprintf(stderr, "Warning: No hvigor path specified, skipping build\n");
        fprintf(stderr, "Use --hvigor <path> or set QT_HARMONYOS_HVIGOR\n");
        return true; // Not fatal
    }

    if (options.verbose)
        fprintf(stdout, "Building HarmonyOS HAP package\n");

    // Check if hvigorw exists
    if (!QFile::exists(hvigorPath)) {
        fprintf(stderr, "Error: hvigorw not found at: %s\n", qPrintable(hvigorPath));
        return false;
    }

    // Determine build task and build mode. hvigor's assembleHap builds in debug
    // mode unless the mode is passed explicitly.
    QString buildTask = "assembleHap"_L1;
    QString buildMode = options.releaseMode ? "release"_L1 : "debug"_L1;

    if (options.verbose) {
        fprintf(stdout, "  Build mode: %s\n", qPrintable(buildMode));
        fprintf(stdout, "  Running: %s %s\n", qPrintable(hvigorPath), qPrintable(buildTask));
    }

    // Execute hvigorw
    QProcessExt process;
    process.setWorkingDirectory(options.outputDirectory);
    process.setProcessChannelMode(QProcess::MergedChannels);

    QStringList arguments;
    arguments << buildTask << "-p"_L1 << ("buildMode="_L1 + buildMode);
    // The hvigor daemon outlives the build and, on Windows, inherits our output
    // pipe; QProcess then never sees EOF and this call hangs. Build daemon-less.
    arguments << "--no-daemon"_L1;

    process.start(hvigorPath, arguments);

    if (!process.waitForStarted()) {
        fprintf(stderr, "Failed to start hvigorw\n");
        return false;
    }

    // Show output in real-time if verbose
    while (process.state() != QProcess::NotRunning) {
        if (!process.waitForReadyRead(1000)) {
            // Timeout is OK, just check if process is still running
            if (process.state() == QProcess::NotRunning)
                break;
            continue;
        }

        if (options.verbose) {
            QByteArray output = process.readAll();
            fprintf(stdout, "%s", output.constData());
            fflush(stdout);
        }
    }

    // Read any remaining output
    if (options.verbose) {
        QByteArray output = process.readAll();
        if (!output.isEmpty()) {
            fprintf(stdout, "%s", output.constData());
            fflush(stdout);
        }
    }

    if (process.exitCode() != 0) {
        fprintf(stderr, "hvigorw failed with exit code %d\n", process.exitCode());
        if (!options.verbose)
            fprintf(stderr, "Run with --verbose to see build output\n");
        return false;
    }

    if (options.verbose)
        fprintf(stdout, "HAP build completed successfully\n");

    // Try to locate the generated HAP file
    if (hapOutputPath) {
        QString hapSearchPath = options.outputDirectory + "/entry/build/default/outputs/default"_L1;
        QDir hapDir(hapSearchPath);

        if (hapDir.exists()) {
            QStringList hapFiles = hapDir.entryList(QStringList() << "*.hap"_L1, QDir::Files);
            if (!hapFiles.isEmpty()) {
                *hapOutputPath = hapDir.absoluteFilePath(hapFiles.first());
                if (options.verbose)
                    fprintf(stdout, "  Generated HAP: %s\n", qPrintable(*hapOutputPath));
            }
        }
    }

    return true;
}

static bool installToDevice(const Options &options, const QString &hapPath)
{
    if (!options.installApk)
        return true; // Not requested

    if (hapPath.isEmpty()) {
        fprintf(stderr, "Error: Cannot install - HAP file path not found\n");
        return false;
    }

    if (options.verbose)
        fprintf(stdout, "Installing HAP to device\n");

    // Check if hdc is available
    QString hdcPath = "hdc"_L1; // Assume it's in PATH

    // Try to find hdc in SDK
    if (!options.sdkRoot.isEmpty()) {
        QString sdkHdc = options.sdkRoot + "/command-line-tools/hdc"_L1;
        if (QFile::exists(sdkHdc))
            hdcPath = std::move(sdkHdc);
    }

    // Check for connected devices
    QProcessExt checkDevices;
    checkDevices.start(hdcPath, QStringList() << "list"_L1 << "targets"_L1);
    if (!checkDevices.waitForFinished(5000)) {
        fprintf(stderr, "Error: Failed to check for connected devices\n");
        return false;
    }

    QString devicesOutput = QString::fromUtf8(checkDevices.readAllStandardOutput());
    if (devicesOutput.trimmed().isEmpty() || devicesOutput.contains("empty"_L1)) {
        fprintf(stderr, "Error: No HarmonyOS devices connected\n");
        fprintf(stderr, "Connect a device and ensure USB debugging is enabled\n");
        return false;
    }

    if (options.verbose)
        fprintf(stdout, "  Connected devices:\n%s\n", qPrintable(devicesOutput));

    // Uninstall old version if exists
    if (options.verbose)
        fprintf(stdout, "  Uninstalling old version (if exists)\n");

    QProcessExt uninstall;
    uninstall.start(hdcPath, QStringList() << "uninstall"_L1 << options.harmonyOsAppBundleName);
    uninstall.waitForFinished(10000);
    // Don't check result - it's OK if app wasn't installed

    // Install new HAP
    if (options.verbose)
        fprintf(stdout, "  Installing: %s\n", qPrintable(hapPath));

    QProcessExt install;
    install.setProcessChannelMode(QProcess::MergedChannels);
    install.start(hdcPath, QStringList() << "install"_L1 << hapPath);

    if (!install.waitForFinished(60000)) { // 60 second timeout
        fprintf(stderr, "Error: Installation timed out\n");
        return false;
    }

    QString installOutput = QString::fromUtf8(install.readAll());

    if (install.exitCode() != 0) {
        fprintf(stderr, "Error: Installation failed\n");
        fprintf(stderr, "%s\n", qPrintable(installOutput));
        return false;
    }

    if (options.verbose)
        fprintf(stdout, "  Installation output:\n%s\n", qPrintable(installOutput));

    // Launch the app
    if (options.verbose)
        fprintf(stdout, "  Launching application\n");

    QProcessExt launch;
    QStringList launchArgs;
    launchArgs << "shell"_L1 << "aa"_L1 << "start"_L1
               << "-a"_L1 << "EntryAbility"_L1
               << "-b"_L1 << options.harmonyOsAppBundleName;

    launch.start(hdcPath, launchArgs);
    launch.waitForFinished(5000);

    if (launch.exitCode() != 0) {
        fprintf(stderr, "Warning: Failed to launch application\n");
        // Not fatal
    }

    fprintf(stdout, "Successfully installed and launched application on device\n");

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("harmonydeployqt"_L1);
    QCoreApplication::setApplicationVersion("1.0"_L1);

    Options options;
    options.timer.start();

    if (!parseCommandLine(app.arguments(), &options))
        return 1;

    if (options.verbose) {
        fprintf(stdout, "Qt HarmonyOS Deployment Tool\n");
        fprintf(stdout, "==============================\n\n");
    }

    if (!readInputConfiguration(&options))
        return 1;

    const QList<TestQmlModule> testQmlModules =
            options.testBundleMode ? findTestQmlModules(options) : QList<TestQmlModule>{};

    if (options.testBundleMode && !verifyUniqueTestQmlModuleDeployDirs(testQmlModules))
        return 1;

    if (options.verbose)
        fprintf(stdout, "\nDeployment process started...\n");

    QString hapOutputPath;

    // Phase 1: Copy template
    if (!copyTemplate(options)) {
        fprintf(stderr, "Failed to copy template\n");
        return 1;
    }

    // Phase 2: Customize template (in test bundle mode, APP_LIBRARY_NAME uses a placeholder)
    if (!customizeTemplate(options)) {
        fprintf(stderr, "Failed to customize template\n");
        return 1;
    }

    // Phase 3: Copy libraries / dependencies (mode-specific)
    QStringList bundledBinaries; // populated only in test bundle mode
    if (options.testBundleMode) {
        if (!copyTestBinaries(options, bundledBinaries)) {
            fprintf(stderr, "Failed to copy test binaries\n");
            return 1;
        }
        if (!copyAllQtLibs(options)) {
            fprintf(stderr, "Failed to copy Qt libraries\n");
            return 1;
        }
        if (!copyAllQtPlugins(options)) {
            fprintf(stderr, "Failed to copy Qt plugins\n");
            return 1;
        }
        if (!copyAllQmlModules(options)) {
            fprintf(stderr, "Failed to copy QML modules\n");
            return 1;
        }
        if (!copyTestQmlModules(testQmlModules, options)) {
            fprintf(stderr, "Failed to copy test QML modules\n");
            return 1;
        }
    } else {
        if (!copyApplicationBinary(options)) {
            fprintf(stderr, "Failed to copy application binary\n");
            return 1;
        }
        if (!copyProjectLibraries(options)) {
            fprintf(stderr, "Failed to copy project libraries\n");
            return 1;
        }
        QSet<QString> processedLibs;
        if (!detectAndCopyDependencies(options, processedLibs)) {
            fprintf(stderr, "Failed to detect and copy dependencies\n");
            return 1;
        }
        QList<QmlImportInfo> qmlImports = scanQmlImports(options);
        if (!copyQmlFiles(options)) {
            fprintf(stderr, "Failed to copy QML files\n");
            return 1;
        }
        if (!copyQmlImports(options, qmlImports, processedLibs)) {
            fprintf(stderr, "Failed to copy QML imports\n");
            return 1;
        }
        if (!copyPlugins(options, processedLibs)) {
            fprintf(stderr, "Failed to copy plugins\n");
            return 1;
        }
    }

    if (!copyExtraPlugins(options)) {
        fprintf(stderr, "Failed to copy extra plugins\n");
        return 1;
    }

    if (!injectSigningConfig(options)) {
        fprintf(stderr, "Failed to inject signing configuration\n");
        return 1;
    }

    // Phase 4: Build HAP package
    if (!buildHap(options, &hapOutputPath)) {
        fprintf(stderr, "Failed to build HAP\n");
        return 1;
    }

    // Phase 5: Test bundle finalization
    if (options.testBundleMode) {
        if (!writeTestBinariesList(options, bundledBinaries)) {
            fprintf(stderr, "Failed to write binaries.txt\n");
            return 1;
        }
    }

    // Write dependency file for CMake DEPFILE support
    // Always write depfile (even if HAP build was skipped), using expected output path
    if (!options.depFilePath.isEmpty()) {
        if (hapOutputPath.isEmpty()) {
            // Construct expected HAP path (matches CMake's HAP_OUTPUT_FILE)
            if (options.testBundleMode) {
                hapOutputPath = options.outputDirectory
                    + "/entry/build/default/outputs/default/autotests.hap"_L1;
            } else {
                // Extract target name from binary: libnativeresource_test.so -> nativeresource_test
                QString targetName = QFileInfo(options.applicationBinary).completeBaseName();
                if (targetName.startsWith("lib"_L1))
                    targetName = targetName.mid(3);
                hapOutputPath = options.outputDirectory + "/entry/build/default/outputs/default/"_L1
                              + targetName + ".hap"_L1;
            }
        }
        if (!writeDepfile(options, hapOutputPath))
            fprintf(stderr, "Warning: Failed to write dependency file\n");
    }

    // Phase 6: Install to device (standard mode only)
    if (!options.testBundleMode) {
        if (!installToDevice(options, hapOutputPath)) {
            fprintf(stderr, "Failed to install to device\n");
            return 1;
        }
    }

    fprintf(stdout, "\n==============================================\n");
    fprintf(stdout, "Deployment completed successfully!\n");
    fprintf(stdout, "==============================================\n");
    fprintf(stdout, "Project location: %s\n", qPrintable(options.outputDirectory));

    if (!hapOutputPath.isEmpty())
        fprintf(stdout, "HAP package: %s\n", qPrintable(hapOutputPath));

    if (options.verbose)
        fprintf(stdout, "\nTotal time: %lld ms\n", options.timer.elapsed());

    return 0;
}
