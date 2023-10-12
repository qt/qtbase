// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils.h"
#include "qmlutils.h"
#include "qtmoduleinfo.h"
#include "qtplugininfo.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QList>
#include <QtCore/QOperatingSystemVersion>
#include <QtCore/QSharedPointer>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#else
#define IMAGE_FILE_MACHINE_ARM64 0xaa64
#endif

#include <QtCore/private/qconfig_p.h>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <unordered_map>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QtModuleInfoStore qtModuleEntries;

#define DECLARE_KNOWN_MODULE(name) \
    static size_t Qt##name ## ModuleId = QtModule::InvalidId

DECLARE_KNOWN_MODULE(3DQuick);
DECLARE_KNOWN_MODULE(Core);
DECLARE_KNOWN_MODULE(Designer);
DECLARE_KNOWN_MODULE(DesignerComponents);
DECLARE_KNOWN_MODULE(Gui);
DECLARE_KNOWN_MODULE(Qml);
DECLARE_KNOWN_MODULE(QmlTooling);
DECLARE_KNOWN_MODULE(Quick);
DECLARE_KNOWN_MODULE(WebEngineCore);
DECLARE_KNOWN_MODULE(Widgets);

#define DEFINE_KNOWN_MODULE(name) \
    m[QLatin1String("Qt6" #name)] = &Qt##name ## ModuleId

static void assignKnownModuleIds()
{
    std::unordered_map<QString, size_t *> m;
    DEFINE_KNOWN_MODULE(3DQuick);
    DEFINE_KNOWN_MODULE(Core);
    DEFINE_KNOWN_MODULE(Designer);
    DEFINE_KNOWN_MODULE(DesignerComponents);
    DEFINE_KNOWN_MODULE(Gui);
    DEFINE_KNOWN_MODULE(Qml);
    DEFINE_KNOWN_MODULE(QmlTooling);
    DEFINE_KNOWN_MODULE(Quick);
    DEFINE_KNOWN_MODULE(WebEngineCore);
    DEFINE_KNOWN_MODULE(Widgets);
    for (size_t i = 0; i < qtModuleEntries.size(); ++i) {
        const QtModule &module = qtModuleEntries.moduleById(i);
        auto it = m.find(module.name);
        if (it == m.end())
            continue;
        *(it->second) = i;
    }
}

#undef DECLARE_KNOWN_MODULE
#undef DEFINE_KNOWN_MODULE

static const char webEngineProcessC[] = "QtWebEngineProcess";

static inline QString webProcessBinary(const char *binaryName, Platform p)
{
    const QString webProcess = QLatin1StringView(binaryName);
    return (p & WindowsBased) ? webProcess + QStringLiteral(".exe") : webProcess;
}

static QString moduleNameToOptionName(const QString &moduleName)
{
    QString result = moduleName
            .mid(3)                    // strip the "Qt6" prefix
            .toLower();
    if (result == u"help"_s)
        result.prepend("qt"_L1);
    return result;
}

static QByteArray formatQtModules(const ModuleBitset &mask, bool option = false)
{
    QByteArray result;
    for (const auto &qtModule : qtModuleEntries) {
        if (mask.test(qtModule.id)) {
            if (!result.isEmpty())
                result.append(' ');
            result.append(option
                          ? moduleNameToOptionName(qtModule.name).toUtf8()
                          : qtModule.name.toUtf8());
            if (qtModule.internal)
                result.append("Internal");
        }
    }
    return result;
}

static QString formatQtPlugins(const PluginInformation &pluginInfo)
{
    QString result(u'\n');
    for (const auto &pair : pluginInfo.typeMap()) {
        result += pair.first;
        result += u": \n";
        for (const QString &plugin : pair.second) {
            result += u"    ";
            result += plugin;
            result += u'\n';
        }
    }
    return result;
}

static Platform platformFromMkSpec(const QString &xSpec)
{
    if (xSpec.startsWith("win32-"_L1)) {
        if (xSpec.contains("clang-g++"_L1))
            return WindowsDesktopClangMinGW;
        if (xSpec.contains("clang-msvc++"_L1))
            return WindowsDesktopClangMsvc;
        return xSpec.contains("g++"_L1) ? WindowsDesktopMinGW : WindowsDesktopMsvc;
    }
    return UnknownPlatform;
}

// Helpers for exclusive options, "-foo", "--no-foo"
enum ExlusiveOptionValue {
    OptionAuto,
    OptionEnabled,
    OptionDisabled
};

static ExlusiveOptionValue parseExclusiveOptions(const QCommandLineParser *parser,
                                                 const QCommandLineOption &enableOption,
                                                 const QCommandLineOption &disableOption)
{
    const bool enabled = parser->isSet(enableOption);
    const bool disabled = parser->isSet(disableOption);
    if (enabled) {
        if (disabled) {
            std::wcerr << "Warning: both -" << enableOption.names().first()
                << " and -" << disableOption.names().first() << " were specified, defaulting to -"
                << enableOption.names().first() << ".\n";
        }
        return OptionEnabled;
    }
    return disabled ? OptionDisabled : OptionAuto;
}

struct Options {
    enum DebugDetection {
        DebugDetectionAuto,
        DebugDetectionForceDebug,
        DebugDetectionForceRelease
    };

    bool plugins = true;
    bool libraries = true;
    bool quickImports = true;
    bool translations = true;
    bool systemD3dCompiler = true;
    bool compilerRunTime = false;
    bool softwareRasterizer = true;
    PluginLists pluginSelections;
    Platform platform = WindowsDesktopMsvc;
    ModuleBitset additionalLibraries;
    ModuleBitset disabledLibraries;
    unsigned updateFileFlags = 0;
    QStringList qmlDirectories; // Project's QML files.
    QStringList qmlImportPaths; // Custom QML module locations.
    QString directory;
    QString qtpathsBinary;
    QString translationsDirectory; // Translations target directory
    QStringList languages;
    QString libraryDirectory;
    QString pluginDirectory;
    QString qmlDirectory;
    QStringList binaries;
    JsonOutput *json = nullptr;
    ListOption list = ListNone;
    DebugDetection debugDetection = DebugDetectionAuto;
    bool deployPdb = false;
    bool dryRun = false;
    bool patchQt = true;
    bool ignoreLibraryErrors = false;
    bool deployInsightTrackerPlugin = false;
};

// Return binary to be deployed from folder, ignore pre-existing web engine process.
static inline QString findBinary(const QString &directory, Platform platform)
{
    const QStringList nameFilters = (platform & WindowsBased) ?
        QStringList(QStringLiteral("*.exe")) : QStringList();
    const QFileInfoList &binaries =
        QDir(QDir::cleanPath(directory)).entryInfoList(nameFilters, QDir::Files | QDir::Executable);
    for (const QFileInfo &binaryFi : binaries) {
        const QString binary = binaryFi.fileName();
        if (!binary.contains(QLatin1StringView(webEngineProcessC), Qt::CaseInsensitive)) {
            return binaryFi.absoluteFilePath();
        }
    }
    return QString();
}

static QString msgFileDoesNotExist(const QString & file)
{
    return u'"' + QDir::toNativeSeparators(file) + "\" does not exist."_L1;
}

enum CommandLineParseFlag {
    CommandLineParseError = 0x1,
    CommandLineParseHelpRequested = 0x2
};

static QCommandLineOption createQMakeOption()
{
    return {
        u"qmake"_s,
        u"Use specified qmake instead of qmake from PATH. Deprecated, use qtpaths instead."_s,
        u"path"_s
    };
}

static QCommandLineOption createQtPathsOption()
{
    return {
            u"qtpaths"_s,
            u"Use specified qtpaths.exe instead of qtpaths.exe from PATH."_s,
            u"path"_s
    };
}

static QCommandLineOption createVerboseOption()
{
    return {
            u"verbose"_s,
            u"Verbose level (0-2)."_s,
            u"level"_s
    };
}

static int parseEarlyArguments(const QStringList &arguments, Options *options,
                               QString *errorMessage)
{
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    QCommandLineOption qmakeOption = createQMakeOption();
    parser.addOption(qmakeOption);

    QCommandLineOption qtpathsOption = createQtPathsOption();
    parser.addOption(qtpathsOption);

    QCommandLineOption verboseOption = createVerboseOption();
    parser.addOption(verboseOption);

    // Deliberately don't check for errors. We want to ignore options we don't know about.
    parser.parse(arguments);

    if (parser.isSet(qmakeOption) && parser.isSet(qtpathsOption)) {
        *errorMessage = QStringLiteral("-qmake and -qtpaths are mutually exclusive.");
        return CommandLineParseError;
    }

    if (parser.isSet(qmakeOption) && optVerboseLevel >= 1)
        std::wcerr << "Warning: -qmake option is deprecated. Use -qtpaths instead.\n";

    if (parser.isSet(qtpathsOption) || parser.isSet(qmakeOption)) {
        const QString qtpathsArg = parser.isSet(qtpathsOption) ? parser.value(qtpathsOption)
                                                                : parser.value(qmakeOption);

        const QString qtpathsBinary = QDir::cleanPath(qtpathsArg);
        const QFileInfo fi(qtpathsBinary);
        if (!fi.exists()) {
            *errorMessage = msgFileDoesNotExist(qtpathsBinary);
            return CommandLineParseError;
        }

        if (!fi.isExecutable()) {
            *errorMessage = u'"' + QDir::toNativeSeparators(qtpathsBinary)
                    + QStringLiteral("\" is not an executable.");
            return CommandLineParseError;
        }
        options->qtpathsBinary = qtpathsBinary;
    }

    if (parser.isSet(verboseOption)) {
        bool ok;
        const QString value = parser.value(verboseOption);
        optVerboseLevel = value.toInt(&ok);
        if (!ok || optVerboseLevel < 0) {
            *errorMessage = QStringLiteral("Invalid value \"%1\" passed for verbose level.")
                    .arg(value);
            return CommandLineParseError;
        }
    }

    return 0;
}

static inline int parseArguments(const QStringList &arguments, QCommandLineParser *parser,
                                 Options *options, QString *errorMessage)
{
    using CommandLineOptionPtr = QSharedPointer<QCommandLineOption>;
    using OptionPtrVector = QList<CommandLineOptionPtr>;

    parser->setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser->setApplicationDescription(u"Qt Deploy Tool " QT_VERSION_STR
        "\n\nThe simplest way to use windeployqt is to add the bin directory of your Qt\n"
        "installation (e.g. <QT_DIR\\bin>) to the PATH variable and then run:\n  windeployqt <path-to-app-binary>\n\n"
        "If your application uses Qt Quick, run:\n  windeployqt --qmldir <path-to-app-qml-files> <path-to-app-binary>"_s);
    const QCommandLineOption helpOption = parser->addHelpOption();
    parser->addVersionOption();

    QCommandLineOption dirOption(QStringLiteral("dir"),
                                 QStringLiteral("Use directory instead of binary directory."),
                                 QStringLiteral("directory"));
    parser->addOption(dirOption);

    // Add early options to have them available in the help text.
    parser->addOption(createQMakeOption());
    parser->addOption(createQtPathsOption());

    QCommandLineOption libDirOption(QStringLiteral("libdir"),
                                    QStringLiteral("Copy libraries to path."),
                                    QStringLiteral("path"));
    parser->addOption(libDirOption);

    QCommandLineOption pluginDirOption(QStringLiteral("plugindir"),
                                       QStringLiteral("Copy plugins to path."),
                                       QStringLiteral("path"));
    parser->addOption(pluginDirOption);

    const QCommandLineOption translationDirOption(
        u"translationdir"_s,
        u"Copy translations to path."_s,
        u"path"_s);
    parser->addOption(translationDirOption);

    QCommandLineOption qmlDeployDirOption(QStringLiteral("qml-deploy-dir"),
                                          QStringLiteral("Copy qml files to path."),
                                          QStringLiteral("path"));
    parser->addOption(qmlDeployDirOption);

    QCommandLineOption debugOption(QStringLiteral("debug"),
                                   QStringLiteral("Assume debug binaries."));
    parser->addOption(debugOption);
    QCommandLineOption releaseOption(QStringLiteral("release"),
                                   QStringLiteral("Assume release binaries."));
    parser->addOption(releaseOption);
    QCommandLineOption releaseWithDebugInfoOption(QStringLiteral("release-with-debug-info"),
                                                  QStringLiteral("Assume release binaries with debug information."));
    releaseWithDebugInfoOption.setFlags(QCommandLineOption::HiddenFromHelp); // Deprecated by improved debug detection.
    parser->addOption(releaseWithDebugInfoOption);

    QCommandLineOption deployPdbOption(QStringLiteral("pdb"),
                                       QStringLiteral("Deploy .pdb files (MSVC)."));
    parser->addOption(deployPdbOption);

    QCommandLineOption forceOption(QStringLiteral("force"),
                                    QStringLiteral("Force updating files."));
    parser->addOption(forceOption);

    QCommandLineOption dryRunOption(QStringLiteral("dry-run"),
                                    QStringLiteral("Simulation mode. Behave normally, but do not copy/update any files."));
    parser->addOption(dryRunOption);

    QCommandLineOption noPatchQtOption(QStringLiteral("no-patchqt"),
                                       QStringLiteral("Do not patch the Qt6Core library."));
    parser->addOption(noPatchQtOption);

    QCommandLineOption ignoreErrorOption(QStringLiteral("ignore-library-errors"),
                                         QStringLiteral("Ignore errors when libraries cannot be found."));
    parser->addOption(ignoreErrorOption);

    QCommandLineOption noPluginsOption(QStringLiteral("no-plugins"),
                                       QStringLiteral("Skip plugin deployment."));
    parser->addOption(noPluginsOption);

    QCommandLineOption skipPluginTypesOption(QStringLiteral("skip-plugin-types"),
                                         QStringLiteral("A comma-separated list of plugin types that are not deployed (qmltooling,generic)."),
                                         QStringLiteral("plugin types"));
    parser->addOption(skipPluginTypesOption);

    QCommandLineOption addPluginTypesOption(QStringLiteral("add-plugin-types"),
                                            QStringLiteral("A comma-separated list of plugin types that will be added to deployment (imageformats,iconengines)"),
                                            QStringLiteral("plugin types"));
    parser->addOption(addPluginTypesOption);

    QCommandLineOption includePluginsOption(QStringLiteral("include-plugins"),
                                            QStringLiteral("A comma-separated list of individual plugins that will be added to deployment (scene2d,qjpeg)"),
                                            QStringLiteral("plugins"));
    parser->addOption(includePluginsOption);

    QCommandLineOption excludePluginsOption(QStringLiteral("exclude-plugins"),
                                            QStringLiteral("A comma-separated list of individual plugins that will not be deployed (qsvg,qpdf)"),
                                            QStringLiteral("plugins"));
    parser->addOption(excludePluginsOption);

    QCommandLineOption noLibraryOption(QStringLiteral("no-libraries"),
                                       QStringLiteral("Skip library deployment."));
    parser->addOption(noLibraryOption);

    QCommandLineOption qmlDirOption(QStringLiteral("qmldir"),
                                    QStringLiteral("Scan for QML-imports starting from directory."),
                                    QStringLiteral("directory"));
    parser->addOption(qmlDirOption);

    QCommandLineOption qmlImportOption(QStringLiteral("qmlimport"),
                                       QStringLiteral("Add the given path to the QML module search locations."),
                                       QStringLiteral("directory"));
    parser->addOption(qmlImportOption);

    QCommandLineOption noQuickImportOption(QStringLiteral("no-quick-import"),
                                           QStringLiteral("Skip deployment of Qt Quick imports."));
    parser->addOption(noQuickImportOption);


    QCommandLineOption translationOption(QStringLiteral("translations"),
                                         QStringLiteral("A comma-separated list of languages to deploy (de,fi)."),
                                         QStringLiteral("languages"));
    parser->addOption(translationOption);

    QCommandLineOption noTranslationOption(QStringLiteral("no-translations"),
                                           QStringLiteral("Skip deployment of translations."));
    parser->addOption(noTranslationOption);

    QCommandLineOption noSystemD3DCompilerOption(QStringLiteral("no-system-d3d-compiler"),
                                                 QStringLiteral("Skip deployment of the system D3D compiler."));
    parser->addOption(noSystemD3DCompilerOption);


    QCommandLineOption compilerRunTimeOption(QStringLiteral("compiler-runtime"),
                                             QStringLiteral("Deploy compiler runtime (Desktop only)."));
    parser->addOption(compilerRunTimeOption);

    QCommandLineOption noCompilerRunTimeOption(QStringLiteral("no-compiler-runtime"),
                                             QStringLiteral("Do not deploy compiler runtime (Desktop only)."));
    parser->addOption(noCompilerRunTimeOption);

    QCommandLineOption jsonOption(QStringLiteral("json"),
                                  QStringLiteral("Print to stdout in JSON format."));
    parser->addOption(jsonOption);

    QCommandLineOption suppressSoftwareRasterizerOption(QStringLiteral("no-opengl-sw"),
                                                        QStringLiteral("Do not deploy the software rasterizer library."));
    parser->addOption(suppressSoftwareRasterizerOption);

    QCommandLineOption listOption(QStringLiteral("list"),
                                                "Print only the names of the files copied.\n"
                                                "Available options:\n"
                                                "  source:   absolute path of the source files\n"
                                                "  target:   absolute path of the target files\n"
                                                "  relative: paths of the target files, relative\n"
                                                "            to the target directory\n"
                                                "  mapping:  outputs the source and the relative\n"
                                                "            target, suitable for use within an\n"
                                                "            Appx mapping file"_L1,
                                  QStringLiteral("option"));
    parser->addOption(listOption);

    // Add early option to have it available in the help text.
    parser->addOption(createVerboseOption());

    parser->addPositionalArgument(QStringLiteral("[files]"),
                                  QStringLiteral("Binaries or directory containing the binary."));

    QCommandLineOption deployInsightTrackerOption(QStringLiteral("deploy-insighttracker"),
                                                  QStringLiteral("Deploy insight tracker plugin."));
    // The option will be added to the parser if the module is available (see block below)
    bool insightTrackerModuleAvailable = false;

    OptionPtrVector enabledModuleOptions;
    OptionPtrVector disabledModuleOptions;
    const size_t qtModulesCount = qtModuleEntries.size();
    enabledModuleOptions.reserve(qtModulesCount);
    disabledModuleOptions.reserve(qtModulesCount);
    for (const QtModule &module : qtModuleEntries) {
        const QString option = moduleNameToOptionName(module.name);
        const QString name = module.name;
        if (name == u"InsightTracker") {
            parser->addOption(deployInsightTrackerOption);
            insightTrackerModuleAvailable = true;
        }
        const QString enabledDescription = QStringLiteral("Add ") + name + QStringLiteral(" module.");
        CommandLineOptionPtr enabledOption(new QCommandLineOption(option, enabledDescription));
        parser->addOption(*enabledOption.data());
        enabledModuleOptions.append(enabledOption);
        const QString disabledDescription = QStringLiteral("Remove ") + name + QStringLiteral(" module.");
        CommandLineOptionPtr disabledOption(new QCommandLineOption(QStringLiteral("no-") + option,
                                                                   disabledDescription));
        disabledModuleOptions.append(disabledOption);
        parser->addOption(*disabledOption.data());
    }

    const bool success = parser->parse(arguments);
    if (parser->isSet(helpOption))
        return CommandLineParseHelpRequested;
    if (!success) {
        *errorMessage = parser->errorText();
        return CommandLineParseError;
    }

    options->libraryDirectory = parser->value(libDirOption);
    options->pluginDirectory = parser->value(pluginDirOption);
    options->translationsDirectory = parser->value(translationDirOption);
    options->qmlDirectory = parser->value(qmlDeployDirOption);
    options->plugins = !parser->isSet(noPluginsOption);
    options->libraries = !parser->isSet(noLibraryOption);
    options->translations = !parser->isSet(noTranslationOption);
    if (parser->isSet(translationOption))
        options->languages = parser->value(translationOption).split(u',');
    options->systemD3dCompiler = !parser->isSet(noSystemD3DCompilerOption);
    options->quickImports = !parser->isSet(noQuickImportOption);

    // default to deployment of compiler runtime for windows desktop configurations
    if (options->platform == WindowsDesktopMinGW || options->platform == WindowsDesktopMsvc
            || parser->isSet(compilerRunTimeOption))
        options->compilerRunTime = true;
    if (parser->isSet(noCompilerRunTimeOption))
        options->compilerRunTime = false;

    if (options->compilerRunTime && options->platform != WindowsDesktopMinGW && options->platform != WindowsDesktopMsvc) {
        *errorMessage = QStringLiteral("Deployment of the compiler runtime is implemented for Desktop MSVC/g++ only.");
        return CommandLineParseError;
    }

    if (parser->isSet(skipPluginTypesOption))
        options->pluginSelections.disabledPluginTypes = parser->value(skipPluginTypesOption).split(u',');

    if (parser->isSet(addPluginTypesOption))
        options->pluginSelections.enabledPluginTypes = parser->value(addPluginTypesOption).split(u',');

    if (parser->isSet(includePluginsOption))
        options->pluginSelections.includedPlugins = parser->value(includePluginsOption).split(u',');

    if (parser->isSet(excludePluginsOption))
        options->pluginSelections.excludedPlugins = parser->value(excludePluginsOption).split(u',');

    if (parser->isSet(releaseWithDebugInfoOption))
        std::wcerr << "Warning: " << releaseWithDebugInfoOption.names().first() << " is obsolete.";

    switch (parseExclusiveOptions(parser, debugOption, releaseOption)) {
    case OptionAuto:
        break;
    case OptionEnabled:
        options->debugDetection = Options::DebugDetectionForceDebug;
        break;
    case OptionDisabled:
        options->debugDetection = Options::DebugDetectionForceRelease;
        break;
    }

    if (parser->isSet(deployPdbOption)) {
        if (options->platform.testFlag(WindowsBased) && !options->platform.testFlag(MinGW))
            options->deployPdb = true;
        else
            std::wcerr << "Warning: --" << deployPdbOption.names().first() << " is not supported on this platform.\n";
    }

    if (parser->isSet(suppressSoftwareRasterizerOption))
        options->softwareRasterizer = false;

    if (parser->isSet(forceOption))
        options->updateFileFlags |= ForceUpdateFile;
    if (parser->isSet(dryRunOption)) {
        options->dryRun = true;
        options->updateFileFlags |= SkipUpdateFile;
    }

    options->patchQt = !parser->isSet(noPatchQtOption);
    options->ignoreLibraryErrors = parser->isSet(ignoreErrorOption);
    if (insightTrackerModuleAvailable)
        options->deployInsightTrackerPlugin = parser->isSet(deployInsightTrackerOption);

    for (const QtModule &module : qtModuleEntries) {
        if (parser->isSet(*enabledModuleOptions.at(module.id)))
            options->additionalLibraries[module.id] = 1;
        if (parser->isSet(*disabledModuleOptions.at(module.id)))
            options->disabledLibraries[module.id] = 1;
    }

    // Add some dependencies
    if (options->additionalLibraries.test(QtQuickModuleId))
        options->additionalLibraries[QtQmlModuleId] = 1;
    if (options->additionalLibraries.test(QtDesignerComponentsModuleId))
        options->additionalLibraries[QtDesignerModuleId] = 1;

    if (parser->isSet(listOption)) {
        const QString value = parser->value(listOption);
        if (value == QStringLiteral("source")) {
            options->list = ListSource;
        } else if (value == QStringLiteral("target")) {
            options->list = ListTarget;
        } else if (value == QStringLiteral("relative")) {
            options->list = ListRelative;
        } else if (value == QStringLiteral("mapping")) {
            options->list = ListMapping;
        } else {
            *errorMessage = QStringLiteral("Please specify a valid option for -list (source, target, relative, mapping).");
            return CommandLineParseError;
        }
    }

    if (parser->isSet(jsonOption) || options->list) {
        optVerboseLevel = 0;
        options->json = new JsonOutput;
    }

    const QStringList posArgs = parser->positionalArguments();
    if (posArgs.isEmpty()) {
        *errorMessage = QStringLiteral("Please specify the binary or folder.");
        return CommandLineParseError | CommandLineParseHelpRequested;
    }

    if (parser->isSet(dirOption))
        options->directory = parser->value(dirOption);

    if (parser->isSet(qmlDirOption))
        options->qmlDirectories = parser->values(qmlDirOption);

    if (parser->isSet(qmlImportOption))
        options->qmlImportPaths = parser->values(qmlImportOption);

    const QString &file = posArgs.front();
    const QFileInfo fi(QDir::cleanPath(file));
    if (!fi.exists()) {
        *errorMessage = msgFileDoesNotExist(file);
        return CommandLineParseError;
    }

    if (!options->directory.isEmpty() && !fi.isFile()) { // -dir was specified - expecting file.
        *errorMessage = u'"' + file + QStringLiteral("\" is not an executable file.");
        return CommandLineParseError;
    }

    if (fi.isFile()) {
        options->binaries.append(fi.absoluteFilePath());
        if (options->directory.isEmpty())
            options->directory = fi.absolutePath();
    } else {
        const QString binary = findBinary(fi.absoluteFilePath(), options->platform);
        if (binary.isEmpty()) {
            *errorMessage = QStringLiteral("Unable to find binary in \"") + file + u'"';
            return CommandLineParseError;
        }
        options->directory = fi.absoluteFilePath();
        options->binaries.append(binary);
    } // directory.

    // Remaining files or plugin directories
    bool multipleDirs = false;
    for (int i = 1; i < posArgs.size(); ++i) {
        const QFileInfo fi(QDir::cleanPath(posArgs.at(i)));
        const QString path = fi.absoluteFilePath();
        if (!fi.exists()) {
            *errorMessage = msgFileDoesNotExist(path);
            return CommandLineParseError;
        }
        if (fi.isDir()) {
            const QStringList libraries =
                findSharedLibraries(QDir(path), options->platform, MatchDebugOrRelease, QString());
            for (const QString &library : libraries)
                options->binaries.append(path + u'/' + library);
        } else {
            if (fi.absolutePath() != options->directory)
                multipleDirs = true;
            options->binaries.append(path);
        }
    }
    if (multipleDirs)
        std::wcerr << "Warning: using binaries from different directories\n";
    if (options->translationsDirectory.isEmpty())
        options->translationsDirectory = options->directory + "/translations"_L1;
    return 0;
}

// Simple line wrapping at 80 character boundaries.
static inline QString lineBreak(QString s)
{
    for (qsizetype i = 80; i < s.size(); i += 80) {
        const qsizetype lastBlank = s.lastIndexOf(u' ', i);
        if (lastBlank >= 0) {
            s[lastBlank] = u'\n';
            i = lastBlank + 1;
        }
    }
    return s;
}

static inline QString helpText(const QCommandLineParser &p, const PluginInformation &pluginInfo)
{
    QString result = p.helpText();
    // Replace the default-generated text which is too long by a short summary
    // explaining how to enable single libraries.
    if (qtModuleEntries.size() == 0)
        return result;
    const QtModule &firstModule = qtModuleEntries.moduleById(0);
    const QString firstModuleOption = moduleNameToOptionName(firstModule.name);
    const qsizetype moduleStart = result.indexOf("\n  --"_L1 + firstModuleOption);
    const qsizetype argumentsStart = result.lastIndexOf("\nArguments:"_L1);
    if (moduleStart >= argumentsStart)
        return result;
    QString moduleHelp;
    moduleHelp +=
        "\n\nQt libraries can be added by passing their name (-xml) or removed by passing\n"
        "the name prepended by --no- (--no-xml). Available libraries:\n"_L1;
    ModuleBitset mask;
    moduleHelp += lineBreak(QString::fromLatin1(formatQtModules(mask.set(), true)));
    moduleHelp += u"\n\n";
    moduleHelp +=
            u"Qt plugins can be included or excluded individually or by type.\n"
            u"To deploy or block plugins individually, use the --include-plugins\n"
            u"and --exclude-plugins options (--include-plugins qjpeg,qsvgicon)\n"
            u"You can also use the --skip-plugin-types or --add-plugin-types to\n"
            u"achieve similar results with entire plugin groups, like imageformats, e.g.\n"
            u"(--add-plugin-types imageformats,iconengines). Exclusion always takes\n"
            u"precedence over inclusion, and types take precedence over specific plugins.\n"
            u"For example, including qjpeg, but skipping imageformats, will NOT deploy qjpeg.\n"
            u"\nDetected available plugins:\n";
    moduleHelp += formatQtPlugins(pluginInfo);
    result.replace(moduleStart, argumentsStart - moduleStart, moduleHelp);
    return result;
}

static inline bool isQtModule(const QString &libName)
{
    // Match Standard modules named Qt6XX.dll
    if (libName.size() < 3 || !libName.startsWith("Qt"_L1, Qt::CaseInsensitive))
        return false;
    const QChar version = libName.at(2);
    return version.isDigit() && (version.toLatin1() - '0') == QT_VERSION_MAJOR;
}

// Helper for recursively finding all dependent Qt libraries.
static bool findDependentQtLibraries(const QString &qtBinDir, const QString &binary, Platform platform,
                                     QString *errorMessage, QStringList *result,
                                     unsigned *wordSize = nullptr, bool *isDebug = nullptr,
                                     unsigned short *machineArch = nullptr,
                                     int *directDependencyCount = nullptr, int recursionDepth = 0)
{
    QStringList dependentLibs;
    if (directDependencyCount)
        *directDependencyCount = 0;
    if (!readPeExecutable(binary, errorMessage, &dependentLibs, wordSize, isDebug,
                          platform == WindowsDesktopMinGW, machineArch)) {
        errorMessage->prepend("Unable to find dependent libraries of "_L1 +
                              QDir::toNativeSeparators(binary) + " :"_L1);
        return false;
    }
    // Filter out the Qt libraries. Note that depends.exe finds libs from optDirectory if we
    // are run the 2nd time (updating). We want to check against the Qt bin dir libraries
    const int start = result->size();
    for (const QString &lib : std::as_const(dependentLibs)) {
        if (isQtModule(lib)) {
            const QString path = normalizeFileName(qtBinDir + u'/' + QFileInfo(lib).fileName());
            if (!result->contains(path))
                result->append(path);
        }
    }
    const int end = result->size();
    if (directDependencyCount)
        *directDependencyCount = end - start;
    // Recurse
    for (int i = start; i < end; ++i)
        if (!findDependentQtLibraries(qtBinDir, result->at(i), platform, errorMessage, result,
                                      nullptr, nullptr, nullptr, nullptr, recursionDepth + 1))
            return false;
    return true;
}

// Base class to filter debug/release Windows DLLs for functions to be passed to updateFile().
// Tries to pre-filter by namefilter and does check via PE.
class DllDirectoryFileEntryFunction {
public:
    explicit DllDirectoryFileEntryFunction(Platform platform,
                                           DebugMatchMode debugMatchMode, const QString &prefix = QString()) :
        m_platform(platform), m_debugMatchMode(debugMatchMode), m_prefix(prefix) {}

    QStringList operator()(const QDir &dir) const
        { return findSharedLibraries(dir, m_platform, m_debugMatchMode, m_prefix); }

private:
    const Platform m_platform;
    const DebugMatchMode m_debugMatchMode;
    const QString m_prefix;
};

static QString pdbFileName(QString libraryFileName)
{
    const qsizetype lastDot = libraryFileName.lastIndexOf(u'.') + 1;
    if (lastDot <= 0)
        return QString();
    libraryFileName.replace(lastDot, libraryFileName.size() - lastDot, "pdb"_L1);
    return libraryFileName;
}
static inline QStringList qmlCacheFileFilters()
{
    return QStringList() << QStringLiteral("*.jsc") << QStringLiteral("*.qmlc");
}

// File entry filter function for updateFile() that returns a list of files for
// QML import trees: DLLs (matching debug) and .qml/,js, etc.
class QmlDirectoryFileEntryFunction {
public:
    enum Flags {
        DeployPdb = 0x1,
        SkipSources = 0x2
    };

    explicit QmlDirectoryFileEntryFunction(
        const QString &moduleSourcePath, Platform platform, DebugMatchMode debugMatchMode, unsigned flags)
        : m_flags(flags), m_qmlNameFilter(QmlDirectoryFileEntryFunction::qmlNameFilters(flags))
        , m_dllFilter(platform, debugMatchMode), m_moduleSourcePath(moduleSourcePath)
    {}

    QStringList operator()(const QDir &dir) const
    {
        if (moduleSourceDir(dir).canonicalPath() != m_moduleSourcePath) {
            // If we're in a different module, return nothing.
            return {};
        }

        QStringList result;
        const QStringList &libraries = m_dllFilter(dir);
        for (const QString &library : libraries) {
            result.append(library);
            if (m_flags & DeployPdb) {
                const QString pdb = pdbFileName(library);
                if (QFileInfo(dir.absoluteFilePath(pdb)).isFile())
                    result.append(pdb);
            }
        }
        result.append(m_qmlNameFilter(dir));
        return result;
    }

private:
    static QDir moduleSourceDir(const QDir &dir)
    {
        QDir moduleSourceDir = dir;
        while (!moduleSourceDir.exists(QStringLiteral("qmldir"))) {
            if (!moduleSourceDir.cdUp()) {
                return {};
            }
        }
        return moduleSourceDir;
    }

    static inline QStringList qmlNameFilters(unsigned flags)
    {
        QStringList result;
        result << QStringLiteral("qmldir") << QStringLiteral("*.qmltypes")
           << QStringLiteral("*.frag") << QStringLiteral("*.vert") // Shaders
           << QStringLiteral("*.ttf");
        if (!(flags & SkipSources)) {
            result << QStringLiteral("*.js") << QStringLiteral("*.qml") << QStringLiteral("*.png");
            result.append(qmlCacheFileFilters());
        }
        return result;
    }

    const unsigned m_flags;
    NameFilterFileEntryFunction m_qmlNameFilter;
    DllDirectoryFileEntryFunction m_dllFilter;
    QString m_moduleSourcePath;
};

static qint64 qtModule(QString module, const QString &infix)
{
    // Match needle 'path/Qt6Core<infix><d>.dll' or 'path/libQt6Core<infix>.so.5.0'
    const qsizetype lastSlashPos = module.lastIndexOf(u'/');
    if (lastSlashPos > 0)
        module.remove(0, lastSlashPos + 1);
    if (module.startsWith("lib"_L1))
        module.remove(0, 3);
    int endPos = infix.isEmpty() ? -1 : module.lastIndexOf(infix);
    if (endPos == -1)
        endPos = module.indexOf(u'.'); // strip suffixes '.so.5.0'.
    if (endPos > 0)
        module.truncate(endPos);
    // That should leave us with 'Qt6Core<d>'.
    for (const auto &qtModule : qtModuleEntries) {
        const QString &libraryName = qtModule.name;
        if (module == libraryName
            || (module.size() == libraryName.size() + 1 && module.startsWith(libraryName))) {
            return qtModule.id;
        }
    }
    std::wcerr << "Warning: module " << qPrintable(module) << " could not be found\n";
    return -1;
}

// Return the path if a plugin is to be deployed
static QString deployPlugin(const QString &plugin, const QDir &subDir, const bool dueToModule,
                            const DebugMatchMode &debugMatchMode, ModuleBitset *usedQtModules,
                            const ModuleBitset &disabledQtModules,
                            const PluginLists &pluginSelections, const QString &libraryLocation,
                            const QString &infix, Platform platform,
                            bool deployInsightTrackerPlugin)
{
    const QString subDirName = subDir.dirName();
    // Filter out disabled plugins
    if (pluginSelections.disabledPluginTypes.contains(subDirName)) {
        std::wcout << "Skipping plugin " << plugin << " due to skipped plugin type " << subDirName << '\n';
        return {};
    }
    if (subDirName == u"generic" && plugin.contains(u"qinsighttracker")
        && !deployInsightTrackerPlugin) {
        std::wcout << "Skipping plugin " << plugin
                   << ". Use -deploy-insighttracker if you want to use it.\n";
        return {};
    }

    const int dotIndex = plugin.lastIndexOf(u'.');
    // Strip the .dll from the name, and an additional 'd' if it's a debug library with the 'd'
    // suffix
    const int stripIndex = debugMatchMode == MatchDebug && platformHasDebugSuffix(platform)
            ? dotIndex - 1
            : dotIndex;
    const QString pluginName = plugin.first(stripIndex);

    if (pluginSelections.excludedPlugins.contains(pluginName)) {
        std::wcout << "Skipping plugin " << plugin << " due to exclusion option" << '\n';
        return {};
    }

    const QString pluginPath = subDir.absoluteFilePath(plugin);

    // If dueToModule is false, check if the user included the plugin or the entire type. In the
    // former's case, only deploy said plugin and not all plugins of that type.
    const bool requiresPlugin = pluginSelections.includedPlugins.contains(pluginName)
            || pluginSelections.enabledPluginTypes.contains(subDirName);
    if (!dueToModule && !requiresPlugin)
        return {};

    // Deploy QUiTools plugins as is without further dependency checking.
    // The user needs to ensure all required libraries are present (would
    // otherwise pull QtWebEngine for its plugin).
    if (subDirName == u"designer")
        return pluginPath;

    QStringList dependentQtLibs;
    ModuleBitset neededModules;
    QString errorMessage;
    if (findDependentQtLibraries(libraryLocation, pluginPath, platform,
                                 &errorMessage, &dependentQtLibs)) {
        for (int d = 0; d < dependentQtLibs.size(); ++d) {
            const qint64 module = qtModule(dependentQtLibs.at(d), infix);
            if (module >= 0)
                neededModules[module] = 1;
        }
    } else {
        std::wcerr << "Warning: Cannot determine dependencies of "
            << QDir::toNativeSeparators(pluginPath) << ": " << errorMessage << '\n';
    }

    ModuleBitset missingModules;
    missingModules = neededModules & disabledQtModules;
    if (missingModules.any()) {
        if (optVerboseLevel) {
            std::wcout << "Skipping plugin " << plugin
                << " due to disabled dependencies ("
                << formatQtModules(missingModules).constData() << ").\n";
        }
        return {};
    }

    missingModules = (neededModules & ~*usedQtModules);
    if (missingModules.any()) {
        *usedQtModules |= missingModules;
        if (optVerboseLevel) {
            std::wcout << "Adding " << formatQtModules(missingModules).constData()
                << " for " << plugin << '\n';
        }
    }
    return pluginPath;
}

static bool needsPluginType(const QString &subDirName, const PluginInformation &pluginInfo,
                            const PluginLists &pluginSelections)
{
    bool needsTypeForPlugin = false;
    for (const QString &plugin: pluginSelections.includedPlugins) {
        if (pluginInfo.isTypeForPlugin(subDirName, plugin))
            needsTypeForPlugin = true;
    }
    return (pluginSelections.enabledPluginTypes.contains(subDirName) || needsTypeForPlugin);
}

QStringList findQtPlugins(ModuleBitset *usedQtModules, const ModuleBitset &disabledQtModules,
                          const PluginInformation &pluginInfo, const PluginLists &pluginSelections,
                          const QString &qtPluginsDirName, const QString &libraryLocation,
                          const QString &infix, DebugMatchMode debugMatchModeIn, Platform platform,
                          QString *platformPlugin, bool deployInsightTrackerPlugin)
{
    if (qtPluginsDirName.isEmpty())
        return QStringList();
    QDir pluginsDir(qtPluginsDirName);
    QStringList result;
    const QFileInfoList &pluginDirs = pluginsDir.entryInfoList(QStringList(u"*"_s), QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDirFi : pluginDirs) {
        const QString subDirName = subDirFi.fileName();
        const size_t module = qtModuleEntries.moduleIdForPluginType(subDirName);
        if (module == QtModule::InvalidId) {
            if (optVerboseLevel > 1) {
                std::wcerr << "No Qt module found for plugin type \"" << subDirName << "\".\n";
            }
            continue;
        }
        const bool dueToModule = usedQtModules->test(module);
        if (dueToModule || needsPluginType(subDirName, pluginInfo, pluginSelections)) {
            const DebugMatchMode debugMatchMode = (module == QtWebEngineCoreModuleId)
                ? MatchDebugOrRelease // QTBUG-44331: Debug detection does not work for webengine, deploy all.
                : debugMatchModeIn;
            QDir subDir(subDirFi.absoluteFilePath());

            // Filter for platform or any.
            QString filter;
            const bool isPlatformPlugin = subDirName == "platforms"_L1;
            if (isPlatformPlugin) {
                filter = QStringLiteral("qwindows");
                if (!infix.isEmpty())
                    filter += infix;
            } else {
                filter = u"*"_s;
            }
            const QStringList plugins =
                    findSharedLibraries(subDir, platform, debugMatchMode, filter);
            for (const QString &plugin : plugins) {
                const QString pluginPath =
                        deployPlugin(plugin, subDir, dueToModule, debugMatchMode, usedQtModules,
                                     disabledQtModules, pluginSelections, libraryLocation, infix,
                                     platform, deployInsightTrackerPlugin);
                if (!pluginPath.isEmpty()) {
                    if (isPlatformPlugin)
                        *platformPlugin = subDir.absoluteFilePath(plugin);
                    result.append(pluginPath);
                }
            } // for filter
        } // type matches
    } // for plugin folder
    return result;
}

static QStringList translationNameFilters(const ModuleBitset &modules, const QString &prefix)
{
    QStringList result;
    for (const auto &qtModule : qtModuleEntries) {
        if (modules.test(qtModule.id) && !qtModule.translationCatalog.isEmpty()) {
            const QString name = qtModule.translationCatalog + u'_' + prefix + ".qm"_L1;
            if (!result.contains(name))
                result.push_back(name);
        }
    }
    return result;
}

static bool deployTranslations(const QString &sourcePath, const ModuleBitset &usedQtModules,
                               const QString &target, const Options &options,
                               QString *errorMessage)
{
    // Find available languages prefixes by checking on qtbase.
    QStringList prefixes;
    QDir sourceDir(sourcePath);
    const QStringList qmFilter = QStringList(QStringLiteral("qtbase_*.qm"));
    const QFileInfoList &qmFiles = sourceDir.entryInfoList(qmFilter);
    for (const QFileInfo &qmFi : qmFiles) {
        const QString prefix = qmFi.baseName().mid(7);
        if (options.languages.isEmpty() || options.languages.contains(prefix))
            prefixes.append(prefix);
    }
    if (prefixes.isEmpty()) {
        std::wcerr << "Warning: Could not find any translations in "
                   << QDir::toNativeSeparators(sourcePath) << " (developer build?)\n.";
        return true;
    }
    // Run lconvert to concatenate all files into a single named "qt_<prefix>.qm" in the application folder
    // Use QT_INSTALL_TRANSLATIONS as working directory to keep the command line short.
    const QString absoluteTarget = QFileInfo(target).absoluteFilePath();
    const QString binary = QStringLiteral("lconvert");
    QStringList arguments;
    for (const QString &prefix : std::as_const(prefixes)) {
        arguments.clear();
        const QString targetFile = QStringLiteral("qt_") + prefix + QStringLiteral(".qm");
        arguments.append(QStringLiteral("-o"));
        const QString targetFilePath = absoluteTarget + u'/' + targetFile;
        if (options.json)
            options.json->addFile(sourcePath +  u'/' + targetFile, absoluteTarget);
        arguments.append(QDir::toNativeSeparators(targetFilePath));
        const QStringList translationFilters = translationNameFilters(usedQtModules, prefix);
        if (translationFilters.isEmpty()){
            std::wcerr << "Warning: translation catalogs are all empty, skipping translation deployment\n";
            return true;
        }
        const QFileInfoList &langQmFiles = sourceDir.entryInfoList(translationFilters);
        for (const QFileInfo &langQmFileFi : langQmFiles) {
            if (options.json) {
                options.json->addFile(langQmFileFi.absoluteFilePath(),
                                      absoluteTarget);
            }
            arguments.append(langQmFileFi.fileName());
        }
        if (optVerboseLevel)
            std::wcout << "Creating " << targetFile << "...\n";
        unsigned long exitCode;
        if ((options.updateFileFlags & SkipUpdateFile) == 0
            && (!runProcess(binary, arguments, sourcePath, &exitCode, nullptr, nullptr, errorMessage)
                || exitCode)) {
            return false;
        }
    } // for prefixes.
    return true;
}

struct DeployResult
{
    operator bool() const { return success; }

    bool success = false;
    bool isDebug = false;
    ModuleBitset directlyUsedQtLibraries;
    ModuleBitset usedQtLibraries;
    ModuleBitset deployedQtLibraries;
};

static QString libraryPath(const QString &libraryLocation, const char *name,
                           const QString &infix, Platform platform, bool debug)
{
    QString result = libraryLocation + u'/';
    result += QLatin1StringView(name);
    result += infix;
    if (debug && platformHasDebugSuffix(platform))
        result += u'd';
    result += sharedLibrarySuffix();
    return result;
}

static QString vcDebugRedistDir() { return QStringLiteral("Debug_NonRedist"); }

static QString vcRedistDir()
{
    const char vcDirVar[] = "VCINSTALLDIR";
    const QChar slash(u'/');
    QString vcRedistDirName = QDir::cleanPath(QFile::decodeName(qgetenv(vcDirVar)));
    if (vcRedistDirName.isEmpty()) {
        std::wcerr << "Warning: Cannot find Visual Studio installation directory, " << vcDirVar
            << " is not set.\n";
        return QString();
    }
    if (!vcRedistDirName.endsWith(slash))
        vcRedistDirName.append(slash);
    vcRedistDirName.append(QStringLiteral("redist/MSVC"));
    if (!QFileInfo(vcRedistDirName).isDir()) {
        std::wcerr << "Warning: Cannot find Visual Studio redist directory, "
            << QDir::toNativeSeparators(vcRedistDirName).toStdWString() << ".\n";
        return QString();
    }
    // Look in reverse order for folder containing the debug redist folder
    const QFileInfoList subDirs =
            QDir(vcRedistDirName)
                    .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::Reversed);
    for (const QFileInfo &f : subDirs) {
        QString path = f.absoluteFilePath();
        if (QFileInfo(path + slash + vcDebugRedistDir()).isDir())
            return path;
        path += QStringLiteral("/onecore");
        if (QFileInfo(path + slash + vcDebugRedistDir()).isDir())
            return path;
    }
    std::wcerr << "Warning: Cannot find Visual Studio redist directory under "
               << QDir::toNativeSeparators(vcRedistDirName).toStdWString() << ".\n";
    return QString();
}

static QStringList findMinGWRuntimePaths(const QString &qtBinDir, Platform platform, const QStringList &runtimeFilters)
{
    //MinGW: Add runtime libraries. Check first for the Qt binary directory, and default to path if nothing is found.
    QStringList result;
    const bool isClang = platform == WindowsDesktopClangMinGW;
    QStringList filters;
    const QString suffix = u'*' + sharedLibrarySuffix();
    for (const auto &minGWRuntime : runtimeFilters)
        filters.append(minGWRuntime + suffix);

    QFileInfoList dlls = QDir(qtBinDir).entryInfoList(filters, QDir::Files);
    if (dlls.isEmpty()) {
        std::wcerr << "Warning: Runtime libraries not found in Qt binary folder, defaulting to looking in path\n";
        const QString binaryPath = isClang ? findInPath("clang++.exe"_L1) : findInPath("g++.exe"_L1);
        if (binaryPath.isEmpty()) {
            std::wcerr << "Warning: Cannot find " << (isClang ? "Clang" : "GCC") << " installation directory, " << (isClang ? "clang++" : "g++") << ".exe must be in the path\n";
            return {};
        }
        const QString binaryFolder = QFileInfo(binaryPath).absolutePath();
        dlls = QDir(binaryFolder).entryInfoList(filters, QDir::Files);
    }

    for (const QFileInfo &dllFi : dlls)
        result.append(dllFi.absoluteFilePath());

    return result;
}

static QStringList compilerRunTimeLibs(const QString &qtBinDir, Platform platform, bool isDebug, unsigned short machineArch)
{
    QStringList result;
    switch (platform) {
    case WindowsDesktopMinGW: {
        const QStringList minGWRuntimes = { "*gcc_"_L1, "*stdc++"_L1, "*winpthread"_L1 };
        result.append(findMinGWRuntimePaths(qtBinDir, platform, minGWRuntimes));
        break;
    }
    case WindowsDesktopClangMinGW: {
        const QStringList clangMinGWRuntimes = { "*unwind"_L1, "*c++"_L1 };
        result.append(findMinGWRuntimePaths(qtBinDir, platform, clangMinGWRuntimes));
        break;
    }
#ifdef Q_OS_WIN
    case WindowsDesktopMsvc: { // MSVC/Desktop: Add redistributable packages.
        QString vcRedistDirName = vcRedistDir();
        if (vcRedistDirName.isEmpty())
             break;
        QStringList redistFiles;
        QDir vcRedistDir(vcRedistDirName);
        const QString machineArchString = getArchString(machineArch);
        if (isDebug) {
            // Append DLLs from Debug_NonRedist\x??\Microsoft.VC<version>.DebugCRT.
            if (vcRedistDir.cd(vcDebugRedistDir()) && vcRedistDir.cd(machineArchString)) {
                const QStringList names = vcRedistDir.entryList(QStringList(QStringLiteral("Microsoft.VC*.DebugCRT")), QDir::Dirs);
                if (!names.isEmpty() && vcRedistDir.cd(names.first())) {
                    const QFileInfoList &dlls = vcRedistDir.entryInfoList(QStringList("*.dll"_L1));
                    for (const QFileInfo &dll : dlls)
                        redistFiles.append(dll.absoluteFilePath());
                }
            }
        } else { // release: Bundle vcredist<>.exe
            QString releaseRedistDir = vcRedistDirName;
            const QStringList countryCodes = vcRedistDir.entryList(QStringList(QStringLiteral("[0-9]*")), QDir::Dirs);
            if (!countryCodes.isEmpty()) // Pre MSVC2017
                releaseRedistDir += u'/' + countryCodes.constFirst();
            QFileInfo fi(releaseRedistDir + "/vc_redist."_L1
                         + machineArchString + ".exe"_L1);
            if (!fi.isFile()) { // Pre MSVC2017/15.5
                fi.setFile(releaseRedistDir + "/vcredist_"_L1
                           + machineArchString + ".exe"_L1);
            }
            if (fi.isFile())
                redistFiles.append(fi.absoluteFilePath());
        }
        if (redistFiles.isEmpty()) {
            std::wcerr << "Warning: Cannot find Visual Studio " << (isDebug ? "debug" : "release")
                       << " redistributable files in " << QDir::toNativeSeparators(vcRedistDirName).toStdWString() << ".\n";
            break;
        }
        result.append(redistFiles);
    }
        break;
#endif // Q_OS_WIN
    default:
        break;
    }
    return result;
}

static inline int qtVersion(const QMap<QString, QString> &qtpathsVariables)
{
    const QString versionString = qtpathsVariables.value(QStringLiteral("QT_VERSION"));
    const QChar dot = u'.';
    const int majorVersion = versionString.section(dot, 0, 0).toInt();
    const int minorVersion = versionString.section(dot, 1, 1).toInt();
    const int patchVersion = versionString.section(dot, 2, 2).toInt();
    return (majorVersion << 16) | (minorVersion << 8) | patchVersion;
}

// Deploy a library along with its .pdb debug info file (MSVC) should it exist.
static bool updateLibrary(const QString &sourceFileName, const QString &targetDirectory,
                          const Options &options, QString *errorMessage)
{
    if (!updateFile(sourceFileName, targetDirectory, options.updateFileFlags, options.json, errorMessage)) {
        if (options.ignoreLibraryErrors) {
            std::wcerr << "Warning: Could not update " << sourceFileName << " :" << *errorMessage << '\n';
            errorMessage->clear();
            return true;
        }
        return false;
    }

    if (options.deployPdb) {
        const QFileInfo pdb(pdbFileName(sourceFileName));
        if (pdb.isFile())
            return updateFile(pdb.absoluteFilePath(), targetDirectory, options.updateFileFlags, nullptr, errorMessage);
    }
    return true;
}

// Find out the ICU version to add the data library icudtXX.dll, which does not
// show as a dependency.
static QString getIcuVersion(const QString &libName)
{
    QString version;
    std::copy_if(libName.cbegin(), libName.cend(), std::back_inserter(version),
                 [](QChar c) { return c.isDigit(); });
    return version;
}

static DeployResult deploy(const Options &options, const QMap<QString, QString> &qtpathsVariables,
                           const PluginInformation &pluginInfo, QString *errorMessage)
{
    DeployResult result;

    const QChar slash = u'/';

    const QString qtBinDir = qtpathsVariables.value(QStringLiteral("QT_INSTALL_BINS"));
    const QString libraryLocation = qtBinDir;
    const QString infix = qtpathsVariables.value(QLatin1StringView(qmakeInfixKey));
    const int version = qtVersion(qtpathsVariables);
    Q_UNUSED(version);

    if (optVerboseLevel > 1)
        std::wcout << "Qt binaries in " << QDir::toNativeSeparators(qtBinDir) << '\n';

    QStringList dependentQtLibs;
    bool detectedDebug;
    unsigned wordSize;
    unsigned short machineArch;
    int directDependencyCount = 0;
    if (!findDependentQtLibraries(libraryLocation, options.binaries.first(), options.platform, errorMessage, &dependentQtLibs, &wordSize,
                                  &detectedDebug, &machineArch, &directDependencyCount)) {
        return result;
    }
    for (int b = 1; b < options.binaries.size(); ++b) {
        if (!findDependentQtLibraries(libraryLocation, options.binaries.at(b), options.platform, errorMessage, &dependentQtLibs,
                                      nullptr, nullptr, nullptr)) {
            return result;
        }
    }

    DebugMatchMode debugMatchMode = MatchDebugOrRelease;
    result.isDebug = false;
    switch (options.debugDetection) {
    case Options::DebugDetectionAuto:
        // Debug detection is only relevant for Msvc/ClangMsvc which have distinct
        // runtimes and binaries. For anything else, use MatchDebugOrRelease
        // since also debug cannot be reliably detect for MinGW.
        if (options.platform.testFlag(Msvc) || options.platform.testFlag(ClangMsvc)) {
            result.isDebug = detectedDebug;
            debugMatchMode = result.isDebug ? MatchDebug : MatchRelease;
        }
        break;
    case Options::DebugDetectionForceDebug:
        result.isDebug = true;
        debugMatchMode = MatchDebug;
        break;
    case Options::DebugDetectionForceRelease:
        debugMatchMode = MatchRelease;
        break;
    }

    // Determine application type, check Quick2 is used by looking at the
    // direct dependencies (do not be fooled by QtWebKit depending on it).
    for (int m = 0; m < dependentQtLibs.size(); ++m) {
        const qint64 module = qtModule(dependentQtLibs.at(m), infix);
        if (module >= 0)
            result.directlyUsedQtLibraries[module] = 1;
    }

    const bool usesQml = result.directlyUsedQtLibraries.test(QtQmlModuleId);
    const bool usesQuick = result.directlyUsedQtLibraries.test(QtQuickModuleId);
    const bool uses3DQuick = result.directlyUsedQtLibraries.test(Qt3DQuickModuleId);
    const bool usesQml2 = !(options.disabledLibraries.test(QtQmlModuleId))
        && (usesQml || usesQuick || uses3DQuick || (options.additionalLibraries.test(QtQmlModuleId)));

    if (optVerboseLevel) {
        std::wcout << QDir::toNativeSeparators(options.binaries.first()) << ' '
                   << wordSize << " bit, " << (result.isDebug ? "debug" : "release")
                   << " executable";
        if (usesQml2)
            std::wcout << " [QML]";
        std::wcout << '\n';
    }

    if (dependentQtLibs.isEmpty()) {
        *errorMessage = QDir::toNativeSeparators(options.binaries.first()) +  QStringLiteral(" does not seem to be a Qt executable.");
        return result;
    }

    // Some Windows-specific checks: Qt5Core depends on ICU when configured with "-icu". Other than
    // that, Qt5WebKit has a hard dependency on ICU.
    if (options.platform.testFlag(WindowsBased))  {
        const QStringList qtLibs = dependentQtLibs.filter(QStringLiteral("Qt6Core"), Qt::CaseInsensitive)
            + dependentQtLibs.filter(QStringLiteral("Qt5WebKit"), Qt::CaseInsensitive);
        for (const QString &qtLib : qtLibs) {
            QStringList icuLibs = findDependentLibraries(qtLib, errorMessage).filter(QStringLiteral("ICU"), Qt::CaseInsensitive);
            if (!icuLibs.isEmpty()) {
                // Find out the ICU version to add the data library icudtXX.dll, which does not show
                // as a dependency.
                const QString icuVersion = getIcuVersion(icuLibs.constFirst());
                if (!icuVersion.isEmpty())  {
                    if (optVerboseLevel > 1)
                        std::wcout << "Adding ICU version " << icuVersion << '\n';
                    QString icuLib = QStringLiteral("icudt") + icuVersion
                            + QLatin1StringView(windowsSharedLibrarySuffix);;
                    // Some packages contain debug dlls of ICU libraries even though it's a C
                    // library and the official packages do not differentiate (QTBUG-87677)
                    if (result.isDebug) {
                        const QString icuLibCandidate = QStringLiteral("icudtd") + icuVersion
                                + QLatin1StringView(windowsSharedLibrarySuffix);
                        if (!findInPath(icuLibCandidate).isEmpty()) {
                            icuLib = icuLibCandidate;
                        }
                    }
                    icuLibs.push_back(icuLib);
                }
                for (const QString &icuLib : std::as_const(icuLibs)) {
                    const QString icuPath = findInPath(icuLib);
                    if (icuPath.isEmpty()) {
                        *errorMessage = QStringLiteral("Unable to locate ICU library ") + icuLib;
                        return result;
                    }
                    dependentQtLibs.push_back(icuPath);
                } // for each icuLib
                break;
            } // !icuLibs.isEmpty()
        } // Qt6Core/Qt6WebKit
    } // Windows

    // Scan Quick2 imports
    QmlImportScanResult qmlScanResult;
    if (options.quickImports && usesQml2) {
        // Custom list of import paths provided by user
        QStringList qmlImportPaths = options.qmlImportPaths;
        // Qt's own QML modules
        qmlImportPaths << qtpathsVariables.value(QStringLiteral("QT_INSTALL_QML"));
        QStringList qmlDirectories = options.qmlDirectories;
        if (qmlDirectories.isEmpty()) {
            const QString qmlDirectory = findQmlDirectory(options.platform, options.directory);
            if (!qmlDirectory.isEmpty())
                qmlDirectories.append(qmlDirectory);
        }
        for (const QString &qmlDirectory : std::as_const(qmlDirectories)) {
            if (optVerboseLevel >= 1)
                std::wcout << "Scanning " << QDir::toNativeSeparators(qmlDirectory) << ":\n";
            const QmlImportScanResult scanResult =
                runQmlImportScanner(qmlDirectory, qmlImportPaths,
                                    result.directlyUsedQtLibraries.test(QtWidgetsModuleId),
                                    options.platform, debugMatchMode, errorMessage);
            if (!scanResult.ok)
                return result;
            qmlScanResult.append(scanResult);
            // Additional dependencies of QML plugins.
            for (const QString &plugin : std::as_const(qmlScanResult.plugins)) {
                if (!findDependentQtLibraries(libraryLocation, plugin, options.platform, errorMessage, &dependentQtLibs, &wordSize, &detectedDebug, &machineArch))
                    return result;
            }
            if (optVerboseLevel >= 1) {
                std::wcout << "QML imports:\n";
                for (const QmlImportScanResult::Module &mod : std::as_const(qmlScanResult.modules)) {
                    std::wcout << "  '" << mod.name << "' "
                               << QDir::toNativeSeparators(mod.sourcePath) << '\n';
                }
                if (optVerboseLevel >= 2) {
                    std::wcout << "QML plugins:\n";
                    for (const QString &p : std::as_const(qmlScanResult.plugins))
                        std::wcout << "  " << QDir::toNativeSeparators(p) << '\n';
                }
            }
        }
    }

    QString platformPlugin;
    // Sort apart Qt 5 libraries in the ones that are represented by the
    // QtModule enumeration (and thus controlled by flags) and others.
    QStringList deployedQtLibraries;
    for (int i = 0 ; i < dependentQtLibs.size(); ++i)  {
        const qint64 module = qtModule(dependentQtLibs.at(i), infix);
        if (module >= 0)
            result.usedQtLibraries[module] = 1;
        else
            deployedQtLibraries.push_back(dependentQtLibs.at(i)); // Not represented by flag.
    }
    result.deployedQtLibraries = (result.usedQtLibraries | options.additionalLibraries) & ~options.disabledLibraries;

    ModuleBitset disabled = options.disabledLibraries;
    if (!usesQml2) {
        disabled[QtQmlModuleId] = 1;
        disabled[QtQuickModuleId] = 1;
    }
    const QStringList plugins = findQtPlugins(
            &result.deployedQtLibraries,
            // For non-QML applications, disable QML to prevent it from being pulled in by the
            // qtaccessiblequick plugin.
            disabled, pluginInfo,
            options.pluginSelections, qtpathsVariables.value(QStringLiteral("QT_INSTALL_PLUGINS")),
            libraryLocation, infix, debugMatchMode, options.platform, &platformPlugin,
            options.deployInsightTrackerPlugin);

    // Apply options flags and re-add library names.
    QString qtGuiLibrary;
    for (const auto &qtModule : qtModuleEntries) {
        if (result.deployedQtLibraries.test(qtModule.id)) {
            const QString library = libraryPath(libraryLocation, qtModule.name.toUtf8(), infix,
                                                options.platform, result.isDebug);
            deployedQtLibraries.append(library);
            if (qtModule.id == QtGuiModuleId)
                qtGuiLibrary = library;
        }
    }

    if (optVerboseLevel >= 1) {
        std::wcout << "Direct dependencies: " << formatQtModules(result.directlyUsedQtLibraries).constData()
                   << "\nAll dependencies   : " << formatQtModules(result.usedQtLibraries).constData()
                   << "\nTo be deployed     : " << formatQtModules(result.deployedQtLibraries).constData() << '\n';
    }

    if (optVerboseLevel > 1)
        std::wcout << "Plugins: " << plugins.join(u',') << '\n';

    if (result.deployedQtLibraries.test(QtGuiModuleId) && platformPlugin.isEmpty()) {
        *errorMessage =QStringLiteral("Unable to find the platform plugin.");
        return result;
    }

    if (options.platform.testFlag(WindowsBased) && !qtGuiLibrary.isEmpty())  {
        const QStringList guiLibraries = findDependentLibraries(qtGuiLibrary, errorMessage);
        const bool dependsOnOpenGl = !guiLibraries.filter(QStringLiteral("opengl32"), Qt::CaseInsensitive).isEmpty();
        if (options.softwareRasterizer && !dependsOnOpenGl) {
            const QFileInfo softwareRasterizer(qtBinDir + slash + QStringLiteral("opengl32sw") + QLatin1StringView(windowsSharedLibrarySuffix));
            if (softwareRasterizer.isFile())
                deployedQtLibraries.append(softwareRasterizer.absoluteFilePath());
        }
        if (options.systemD3dCompiler && machineArch != IMAGE_FILE_MACHINE_ARM64) {
            const QString d3dCompiler = findD3dCompiler(options.platform, qtBinDir, wordSize);
            if (d3dCompiler.isEmpty()) {
                std::wcerr << "Warning: Cannot find any version of the d3dcompiler DLL.\n";
            } else {
                deployedQtLibraries.push_back(d3dCompiler);
            }
        }
    } // Windows

    // Update libraries
    if (options.libraries) {
        const QString targetPath = options.libraryDirectory.isEmpty() ?
            options.directory : options.libraryDirectory;
        QStringList libraries = deployedQtLibraries;
        if (options.compilerRunTime)
            libraries.append(compilerRunTimeLibs(qtBinDir, options.platform, result.isDebug, machineArch));
        for (const QString &qtLib : std::as_const(libraries)) {
            if (!updateLibrary(qtLib, targetPath, options, errorMessage))
                return result;
        }

#if !QT_CONFIG(relocatable)
        if (options.patchQt  && !options.dryRun) {
            const QString qt6CoreName = QFileInfo(libraryPath(libraryLocation, "Qt6Core", infix,
                                                              options.platform, result.isDebug)).fileName();
            if (!patchQtCore(targetPath + u'/' + qt6CoreName, errorMessage)) {
                std::wcerr << "Warning: " << *errorMessage << '\n';
                errorMessage->clear();
            }
        }
#endif // QT_CONFIG(relocatable)
    } // optLibraries

    // Update plugins
    if (options.plugins) {
        const QString targetPath = options.pluginDirectory.isEmpty() ?
            options.directory : options.pluginDirectory;
        QDir dir(targetPath);
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            *errorMessage = "Cannot create "_L1 +
                            QDir::toNativeSeparators(dir.absolutePath()) +  u'.';
            return result;
        }
        for (const QString &plugin : plugins) {
            const QString targetDirName = plugin.section(slash, -2, -2);
            const QString targetPath = dir.absoluteFilePath(targetDirName);
            if (!dir.exists(targetDirName)) {
                if (optVerboseLevel)
                    std::wcout << "Creating directory " << targetPath << ".\n";
                if (!(options.updateFileFlags & SkipUpdateFile) && !dir.mkdir(targetDirName)) {
                    *errorMessage = QStringLiteral("Cannot create ") + targetDirName +  u'.';
                    return result;
                }
            }
            if (!updateLibrary(plugin, targetPath, options, errorMessage))
                return result;
        }
    } // optPlugins

    // Update Quick imports
    // Do not be fooled by QtWebKit.dll depending on Quick into always installing Quick imports
    // for WebKit1-applications. Check direct dependency only.
    if (options.quickImports && usesQml2) {
        const QString targetPath = options.qmlDirectory.isEmpty()
                ? options.directory + QStringLiteral("/qml")
                : options.qmlDirectory;
        if (!createDirectory(targetPath, errorMessage, options.dryRun))
            return result;
        for (const QmlImportScanResult::Module &module : std::as_const(qmlScanResult.modules)) {
            const QString installPath = module.installPath(targetPath);
            if (optVerboseLevel > 1)
                std::wcout << "Installing: '" << module.name
                           << "' from " << module.sourcePath << " to "
                           << QDir::toNativeSeparators(installPath) << '\n';
            if (installPath != targetPath && !createDirectory(installPath, errorMessage, options.dryRun))
                return result;
            unsigned updateFileFlags = options.updateFileFlags
                    | SkipQmlDesignerSpecificsDirectories;
            unsigned qmlDirectoryFileFlags = 0;
            if (options.deployPdb)
                qmlDirectoryFileFlags |= QmlDirectoryFileEntryFunction::DeployPdb;
            if (!updateFile(module.sourcePath, QmlDirectoryFileEntryFunction(module.sourcePath,
                                                                             options.platform,
                                                                             debugMatchMode,
                                                                             qmlDirectoryFileFlags),
                            installPath, updateFileFlags, options.json, errorMessage)) {
                return result;
            }
        }
    } // optQuickImports

    if (options.translations) {
        if (!createDirectory(options.translationsDirectory, errorMessage, options.dryRun))
            return result;
        if (!deployTranslations(qtpathsVariables.value(QStringLiteral("QT_INSTALL_TRANSLATIONS")),
                                result.deployedQtLibraries, options.translationsDirectory, options,
                                errorMessage)) {
            return result;
        }
    }

    result.success = true;
    return result;
}

static bool deployWebProcess(const QMap<QString, QString> &qtpathsVariables, const char *binaryName,
                             const PluginInformation &pluginInfo, const Options &sourceOptions,
                             QString *errorMessage)
{
    // Copy the web process and its dependencies
    const QString webProcess = webProcessBinary(binaryName, sourceOptions.platform);
    const QString webProcessSource = qtpathsVariables.value(QStringLiteral("QT_INSTALL_LIBEXECS"))
            + u'/' + webProcess;
    if (!updateFile(webProcessSource, sourceOptions.directory, sourceOptions.updateFileFlags, sourceOptions.json, errorMessage))
        return false;
    Options options(sourceOptions);
    options.binaries.append(options.directory + u'/' + webProcess);
    options.quickImports = false;
    options.translations = false;
    return deploy(options, qtpathsVariables, pluginInfo, errorMessage);
}

static bool deployWebEngineCore(const QMap<QString, QString> &qtpathsVariables,
                                const PluginInformation &pluginInfo, const Options &options,
                                bool isDebug, QString *errorMessage)
{
    static const char *installDataFiles[] = { "icudtl.dat",
                                              "qtwebengine_devtools_resources.pak",
                                              "qtwebengine_resources.pak",
                                              "qtwebengine_resources_100p.pak",
                                              "qtwebengine_resources_200p.pak",
                                              isDebug ? "v8_context_snapshot.debug.bin"
                                                      : "v8_context_snapshot.bin" };
    QByteArray webEngineProcessName(webEngineProcessC);
    if (isDebug && platformHasDebugSuffix(options.platform))
        webEngineProcessName.append('d');
    if (optVerboseLevel)
        std::wcout << "Deploying: " << webEngineProcessName.constData() << "...\n";
    if (!deployWebProcess(qtpathsVariables, webEngineProcessName, pluginInfo, options, errorMessage))
        return false;
    const QString resourcesSubDir = QStringLiteral("/resources");
    const QString resourcesSourceDir = qtpathsVariables.value(QStringLiteral("QT_INSTALL_DATA"))
            + resourcesSubDir + u'/';
    const QString resourcesTargetDir(options.directory + resourcesSubDir);
    if (!createDirectory(resourcesTargetDir, errorMessage, options.dryRun))
        return false;
    for (auto installDataFile : installDataFiles) {
        if (!updateFile(resourcesSourceDir + QLatin1StringView(installDataFile),
                        resourcesTargetDir, options.updateFileFlags, options.json, errorMessage)) {
            return false;
        }
    }
    const QFileInfo translations(qtpathsVariables.value(QStringLiteral("QT_INSTALL_TRANSLATIONS"))
                                 + QStringLiteral("/qtwebengine_locales"));
    if (!translations.isDir()) {
        std::wcerr << "Warning: Cannot find the translation files of the QtWebEngine module at "
            << QDir::toNativeSeparators(translations.absoluteFilePath()) << ".\n";
        return true;
    }
    if (options.translations) {
        // Copy the whole translations directory.
        return createDirectory(options.translationsDirectory, errorMessage, options.dryRun)
                && updateFile(translations.absoluteFilePath(), options.translationsDirectory,
                              options.updateFileFlags, options.json, errorMessage);
    }
    // Translations have been turned off, but QtWebEngine needs at least one.
    const QFileInfo enUSpak(translations.filePath() + QStringLiteral("/en-US.pak"));
    if (!enUSpak.exists()) {
        std::wcerr << "Warning: Cannot find "
                   << QDir::toNativeSeparators(enUSpak.absoluteFilePath()) << ".\n";
        return true;
    }
    const QString webEngineTranslationsDir = options.translationsDirectory + u'/'
            + translations.fileName();
    if (!createDirectory(webEngineTranslationsDir, errorMessage, options.dryRun))
        return false;
    return updateFile(enUSpak.absoluteFilePath(), webEngineTranslationsDir,
                      options.updateFileFlags, options.json, errorMessage);
}

QT_END_NAMESPACE

QT_USE_NAMESPACE

int main(int argc, char **argv)
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationVersion(QT_VERSION_STR ""_L1);

    const QByteArray qtBinPath = QFile::encodeName(QDir::toNativeSeparators(QCoreApplication::applicationDirPath()));
    QByteArray path = qgetenv("PATH");
    if (!path.contains(qtBinPath)) { // QTBUG-39177, ensure Qt is in the path so that qt.conf is taken into account.
        path.prepend(QDir::listSeparator().toLatin1());
        path.prepend(qtBinPath);
        qputenv("PATH", path);
    }

    Options options;
    QString errorMessage;

    // Early parse the --qmake and --qtpaths options, because they are needed to determine the
    // options that select/deselect Qt modules.
    {
        int result = parseEarlyArguments(QCoreApplication::arguments(), &options, &errorMessage);
        if (result & CommandLineParseError) {
            std::wcerr << "Error: " << errorMessage << "\n";
            return 1;
        }
    }

    const QMap<QString, QString> qtpathsVariables =
            queryQtPaths(options.qtpathsBinary, &errorMessage);
    const QString xSpec = qtpathsVariables.value(QStringLiteral("QMAKE_XSPEC"));
    options.platform = platformFromMkSpec(xSpec);
    if (options.platform == UnknownPlatform) {
        std::wcerr << "Unsupported platform " << xSpec << '\n';
        return 1;
    }

    if (qtpathsVariables.isEmpty() || xSpec.isEmpty()
        || !qtpathsVariables.contains(QStringLiteral("QT_INSTALL_BINS"))) {
        std::wcerr << "Unable to query qtpaths: " << errorMessage << '\n';
        return 1;
    }

    // Read the Qt module information from the Qt installation directory.
    const QString modulesDir
            = qtpathsVariables.value(QLatin1String("QT_INSTALL_ARCHDATA"))
            + QLatin1String("/modules");
    const QString translationsDir
            = qtpathsVariables.value(QLatin1String("QT_INSTALL_TRANSLATIONS"));
    if (!qtModuleEntries.populate(modulesDir, translationsDir, optVerboseLevel > 1,
                                  &errorMessage)) {
        std::wcerr << "Error: " << errorMessage << "\n";
        return 1;
    }
    assignKnownModuleIds();

    // Read the Qt plugin types information from the Qt installation directory.
    PluginInformation pluginInfo{};
    pluginInfo.generateAvailablePlugins(qtpathsVariables, options.platform);

    // Parse the full command line.
    {
        QCommandLineParser parser;
        QString errorMessage;
        const int result = parseArguments(QCoreApplication::arguments(), &parser, &options, &errorMessage);
        if (result & CommandLineParseError)
            std::wcerr << errorMessage << "\n\n";
        if (result & CommandLineParseHelpRequested)
            std::fputs(qPrintable(helpText(parser, pluginInfo)), stdout);
        if (result & CommandLineParseError)
            return 1;
        if (result & CommandLineParseHelpRequested)
            return 0;
    }

    // Create directories
    if (!createDirectory(options.directory, &errorMessage, options.dryRun)) {
        std::wcerr << errorMessage << '\n';
        return 1;
    }
    if (!options.libraryDirectory.isEmpty() && options.libraryDirectory != options.directory
        && !createDirectory(options.libraryDirectory, &errorMessage, options.dryRun)) {
        std::wcerr << errorMessage << '\n';
        return 1;
    }

    const DeployResult result = deploy(options, qtpathsVariables, pluginInfo, &errorMessage);
    if (!result) {
        std::wcerr << errorMessage << '\n';
        return 1;
    }

    if (result.deployedQtLibraries.test(QtWebEngineCoreModuleId)) {
        if (!deployWebEngineCore(qtpathsVariables, pluginInfo, options, result.isDebug,
                                 &errorMessage)) {
            std::wcerr << errorMessage << '\n';
            return 1;
        }
    }

    if (options.json) {
        if (options.list)
            std::fputs(options.json->toList(options.list, options.directory).constData(), stdout);
        else
            std::fputs(options.json->toJson().constData(), stdout);
        delete options.json;
        options.json = nullptr;
    }

    return 0;
}
