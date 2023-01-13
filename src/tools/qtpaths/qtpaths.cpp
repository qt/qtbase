// Copyright (C) 2016 Sune Vuorela <sune@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QStandardPaths>
#include <QHash>
#include <QLibraryInfo>

#include <algorithm>

#include <stdio.h>

#if QT_CONFIG(settings)
#    include <private/qlibraryinfo_p.h>
#    include <qmakelibraryinfo.h>
#    include <propertyprinter.h>
#    include <property.h>
#endif

QT_USE_NAMESPACE

/**
 * Prints the string on stdout and appends a newline
 * \param string printable string
 */
static void message(const QString &string)
{
    fprintf(stdout, "%s\n", qPrintable(string));
}

/**
 * Writes error message and exits 1
 * \param message to write
 */
Q_NORETURN static void error(const QString &message)
{
    fprintf(stderr, "%s\n", qPrintable(message));
    ::exit(EXIT_FAILURE);
}

class StringEnum {
public:
    const char *stringvalue;
    QStandardPaths::StandardLocation enumvalue;
    bool hasappname;

    /**
    * Replace application name by generic name if requested
    */
    QString mapName(const QString &s) const
    {
        return hasappname ? QString(s).replace("qtpaths", "<APPNAME>") : s;
    }
};

static const StringEnum lookupTableData[] = {
    { "AppConfigLocation", QStandardPaths::AppConfigLocation, true },
    { "AppDataLocation", QStandardPaths::AppDataLocation, true },
    { "AppLocalDataLocation", QStandardPaths::AppLocalDataLocation, true },
    { "ApplicationsLocation", QStandardPaths::ApplicationsLocation, false },
    { "CacheLocation", QStandardPaths::CacheLocation, true },
    { "ConfigLocation", QStandardPaths::ConfigLocation, false },
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    { "DataLocation", QStandardPaths::DataLocation, true },
#endif
    { "DesktopLocation", QStandardPaths::DesktopLocation, false },
    { "DocumentsLocation", QStandardPaths::DocumentsLocation, false },
    { "DownloadLocation", QStandardPaths::DownloadLocation, false },
    { "FontsLocation", QStandardPaths::FontsLocation, false },
    { "GenericCacheLocation", QStandardPaths::GenericCacheLocation, false },
    { "GenericConfigLocation", QStandardPaths::GenericConfigLocation, false },
    { "GenericDataLocation", QStandardPaths::GenericDataLocation, false },
    { "HomeLocation", QStandardPaths::HomeLocation, false },
    { "MoviesLocation", QStandardPaths::MoviesLocation, false },
    { "MusicLocation", QStandardPaths::MusicLocation, false },
    { "PicturesLocation", QStandardPaths::PicturesLocation, false },
    { "PublicShareLocation", QStandardPaths::PublicShareLocation, false },
    { "RuntimeLocation", QStandardPaths::RuntimeLocation, false },
    { "TemplatesLocation", QStandardPaths::TemplatesLocation, false },
    { "TempLocation", QStandardPaths::TempLocation, false }
};

/**
 * \return available types as a QStringList.
 */
static QStringList types()
{
    QStringList typelist;
    for (const StringEnum &se : lookupTableData)
        typelist << QString::fromLatin1(se.stringvalue);
    std::sort(typelist.begin(), typelist.end());
    return typelist;
}

/**
 * Tries to parse the location string into a reference to a StringEnum entry or alternatively
 * calls \ref error with a error message
 */
static const StringEnum &parseLocationOrError(const QString &locationString)
{
    for (const StringEnum &se : lookupTableData)
        if (locationString == QLatin1StringView(se.stringvalue))
            return se;

    QString message = QStringLiteral("Unknown location: %1");
    error(message.arg(locationString));
}

/**
 * searches for exactly one remaining argument and returns it.
 * If not found, \ref error is called with a error message.
 * \param parser to ask for remaining arguments
 * \return one extra argument
 */
static QString searchStringOrError(QCommandLineParser *parser)
{
    int positionalArgumentCount = parser->positionalArguments().size();
    if (positionalArgumentCount != 1)
        error(QStringLiteral("Exactly one argument needed as searchitem"));
    return parser->positionalArguments().constFirst();
}

int main(int argc, char **argv)
{
    QString qtconfManualPath;
    QCoreApplication app(argc, argv);
    app.setApplicationVersion(QTPATHS_VERSION_STR);

#ifdef Q_OS_WIN
    const QLatin1Char pathsep(';');
#else
    const QLatin1Char pathsep(':');
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Command line client to QStandardPaths and QLibraryInfo"));
    parser.addPositionalArgument(QStringLiteral("[name]"), QStringLiteral("Name of file or directory"));
    parser.addPositionalArgument(QStringLiteral("[properties]"), QStringLiteral("List of the Qt properties to query by the --qt-query argument."));
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    parser.addHelpOption();
    parser.addVersionOption();

    //setting up options
    QCommandLineOption types(QStringLiteral("types"), QStringLiteral("Available location types."));
    parser.addOption(types);

    QCommandLineOption paths(QStringLiteral("paths"), QStringLiteral("Find paths for <type>."), QStringLiteral("type"));
    parser.addOption(paths);

    QCommandLineOption writablePath(QStringLiteral("writable-path"),
                                    QStringLiteral("Find writable path for <type>."), QStringLiteral("type"));
    parser.addOption(writablePath);

    QCommandLineOption locateDir(QStringList() << QStringLiteral("locate-dir") << QStringLiteral("locate-directory"),
                                 QStringLiteral("Locate directory [name] in <type>."), QStringLiteral("type"));
    parser.addOption(locateDir);

    QCommandLineOption locateDirs(QStringList() << QStringLiteral("locate-dirs") << QStringLiteral("locate-directories"),
                                  QStringLiteral("Locate directories [name] in all paths for <type>."), QStringLiteral("type"));
    parser.addOption(locateDirs);

    QCommandLineOption locateFile(QStringLiteral("locate-file"),
                                  QStringLiteral("Locate file [name] for <type>."), QStringLiteral("type"));
    parser.addOption(locateFile);

    QCommandLineOption locateFiles(QStringLiteral("locate-files"),
                                   QStringLiteral("Locate files [name] in all paths for <type>."), QStringLiteral("type"));
    parser.addOption(locateFiles);

    QCommandLineOption findExe(QStringList() << QStringLiteral("find-exe") << QStringLiteral("find-executable"),
                               QStringLiteral("Find executable with [name]."));
    parser.addOption(findExe);

    QCommandLineOption display(QStringList() << QStringLiteral("display"),
                               QStringLiteral("Prints user readable name for <type>."), QStringLiteral("type"));
    parser.addOption(display);

    QCommandLineOption testmode(QStringList() << QStringLiteral("testmode") << QStringLiteral("test-mode"),
                                QStringLiteral("Use paths specific for unit testing."));
    parser.addOption(testmode);

    QCommandLineOption qtversion(QStringLiteral("qt-version"), QStringLiteral("Qt version."));
    qtversion.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(qtversion);

    QCommandLineOption installprefix(QStringLiteral("install-prefix"), QStringLiteral("Installation prefix for Qt."));
    installprefix.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(installprefix);

    QCommandLineOption bindir(QStringList() << QStringLiteral("binaries-dir") << QStringLiteral("binaries-directory"),
                              QStringLiteral("Location of Qt executables."));
    bindir.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(bindir);

    QCommandLineOption plugindir(QStringList() << QStringLiteral("plugin-dir") << QStringLiteral("plugin-directory"),
                                 QStringLiteral("Location of Qt plugins."));
    plugindir.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOption(plugindir);

    QCommandLineOption query(
            QStringList() << QStringLiteral("qt-query") << QStringLiteral("query"),
            QStringLiteral("List of Qt properties. Can be used standalone or with the "
                           "--query-format and --qtconf options."));
    parser.addOption(query);

    QCommandLineOption queryformat(QStringLiteral("query-format"),
                                   QStringLiteral("Output format for --qt-query.\nSupported formats: qmake (default), json"),
                                   QStringLiteral("format"));
    queryformat.setDefaultValue("qmake");
    parser.addOption(queryformat);

    QCommandLineOption qtconf(QStringLiteral("qtconf"),
                                   QStringLiteral("Path to qt.conf file that will be used to override the queried Qt properties."),
                                   QStringLiteral("path"));
    parser.addOption(qtconf);

    parser.process(app);

    QStandardPaths::setTestModeEnabled(parser.isSet(testmode));

#if QT_CONFIG(settings)
    if (parser.isSet(qtconf)) {
        qtconfManualPath = parser.value(qtconf);
        QLibraryInfoPrivate::qtconfManualPath = &qtconfManualPath;
    }
#endif

    QStringList results;
    if (parser.isSet(qtversion)) {
        QString qtversionstring = QString::fromLatin1(QT_VERSION_STR);
        results << qtversionstring;
    }

    if (parser.isSet(installprefix)) {
        QString path = QLibraryInfo::path(QLibraryInfo::PrefixPath);
        results << path;
    }

    if (parser.isSet(bindir)) {
        QString path = QLibraryInfo::path(QLibraryInfo::BinariesPath);
        results << path;
    }

    if (parser.isSet(plugindir)) {
        QString path = QLibraryInfo::path(QLibraryInfo::PluginsPath);
        results << path;
    }

    if (parser.isSet(types)) {
        QStringList typesList = ::types();
        results << typesList.join('\n');
    }

    QT_WARNING_PUSH
#if defined(Q_CC_GNU_ONLY) && Q_CC_GNU >= 1300 && Q_CC_GNU < 1400
    QT_WARNING_DISABLE_GCC("-Wdangling-reference")
#endif
    if (parser.isSet(display)) {
        const StringEnum &location = parseLocationOrError(parser.value(display));
        QString text = QStandardPaths::displayName(location.enumvalue);
        results << location.mapName(text);
    }

    if (parser.isSet(paths)) {
        const StringEnum &location = parseLocationOrError(parser.value(paths));
        QStringList paths = QStandardPaths::standardLocations(location.enumvalue);
        results << location.mapName(paths.join(pathsep));
    }

    if (parser.isSet(writablePath)) {
        const StringEnum &location = parseLocationOrError(parser.value(writablePath));
        QString path = QStandardPaths::writableLocation(location.enumvalue);
        results << location.mapName(path);
    }

    if (parser.isSet(findExe)) {
        QString searchitem = searchStringOrError(&parser);
        QString path = QStandardPaths::findExecutable(searchitem);
        results << path;
    }

    if (parser.isSet(locateDir)) {
        const StringEnum &location = parseLocationOrError(parser.value(locateDir));
        QString searchitem = searchStringOrError(&parser);
        QString path = QStandardPaths::locate(location.enumvalue, searchitem, QStandardPaths::LocateDirectory);
        results << location.mapName(path);
    }

    if (parser.isSet(locateFile)) {
        const StringEnum &location = parseLocationOrError(parser.value(locateFile));
        QString searchitem = searchStringOrError(&parser);
        QString path = QStandardPaths::locate(location.enumvalue, searchitem, QStandardPaths::LocateFile);
        results << location.mapName(path);
    }

    if (parser.isSet(locateDirs)) {
        const StringEnum &location = parseLocationOrError(parser.value(locateDirs));
        QString searchitem = searchStringOrError(&parser);
        QStringList paths = QStandardPaths::locateAll(location.enumvalue, searchitem, QStandardPaths::LocateDirectory);
        results << location.mapName(paths.join(pathsep));
    }

    if (parser.isSet(locateFiles)) {
        const StringEnum &location = parseLocationOrError(parser.value(locateFiles));
        QString searchitem = searchStringOrError(&parser);
        QStringList paths = QStandardPaths::locateAll(location.enumvalue, searchitem, QStandardPaths::LocateFile);
        results << location.mapName(paths.join(pathsep));
    }
    QT_WARNING_POP

#if !QT_CONFIG(settings)
    if (parser.isSet(query) || parser.isSet(qtconf) || parser.isSet(queryformat)) {
        error(QStringLiteral("--qt-query, --qtconf and --query-format options are not supported. The 'settings' feature is missing."));
    }
#else
    if (parser.isSet(query)) {
        if (!results.isEmpty()) {
            QString errorMessage = QStringLiteral("Several options given, only one is supported at a time.");
            error(errorMessage);
        }

        PropertyPrinter printer;
        if (parser.isSet(queryformat)) {
            QString formatValue = parser.value(queryformat);
            if (formatValue == "json") {
                  printer = jsonPropertyPrinter;
            } else if (formatValue != "qmake") {
                QString errorMessage = QStringLiteral("Invalid output format %1. Supported formats: qmake, json").arg(formatValue);
                error(errorMessage);
            }
        }

        QStringList optionProperties = parser.positionalArguments();
        QMakeProperty prop;
        if (printer) {
            return prop.queryProperty(optionProperties, printer);
        }
        return prop.queryProperty(optionProperties);
    } else if (parser.isSet(queryformat)) {
        error(QStringLiteral("--query-format is set, but --qt-query is not requested."));
    }
#endif

    if (results.isEmpty()) {
        parser.showHelp();
    } else if (results.size() == 1) {
        const QString &item = results.first();
        message(item);
        if (item.isEmpty())
            return EXIT_FAILURE;
    } else {
        QString errorMessage = QStringLiteral("Several options given, only one is supported at a time.");
        error(errorMessage);
    }
    return EXIT_SUCCESS;
}
