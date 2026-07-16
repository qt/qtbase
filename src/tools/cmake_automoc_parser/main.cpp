// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qglobal.h>

#include <cstdio>
#include <cstdlib>
#include <limits>

#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qhash.h>
#include <qjsonarray.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qlist.h>
#include <qmap.h>
#include <qset.h>
#include <qstring.h>
#include <qstack.h>
#include <qdatastream.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

using AutoGenHeaderMap = QMap<QString, QString>;
using AutoGenSourcesList = QList<QString>;

static bool readAutogenInfoJson(AutoGenHeaderMap &headers, AutoGenSourcesList &sources,
                                QStringList &headerExts, QStringList &mocIncludePaths,
                                const QString &config, const QString &autoGenInfoJsonPath)
{
    QFile file(autoGenInfoJsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "Could not open: %s\n", qPrintable(autoGenInfoJsonPath));
        return false;
    }

    const QByteArray contents = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(contents, &error);

    if (error.error != QJsonParseError::NoError) {
        fprintf(stderr, "Failed to parse json file: %s\n", qPrintable(autoGenInfoJsonPath));
        return false;
    }

    QJsonObject rootObject = doc.object();
    QJsonValue headersValue = rootObject.value("HEADERS"_L1);
    QJsonValue sourcesValue = rootObject.value("SOURCES"_L1);
    QJsonValue headerExtValue = rootObject.value("HEADER_EXTENSIONS"_L1);

    if (!headersValue.isArray() || !sourcesValue.isArray() || !headerExtValue.isArray()) {
        fprintf(stderr,
                "%s layout does not match the expected layout. This most likely means that file "
                "format changed or this file is not a product of CMake's AutoGen process.\n",
                qPrintable(autoGenInfoJsonPath));
        return false;
    }

    QJsonArray headersArray = headersValue.toArray();
    QJsonArray sourcesArray = sourcesValue.toArray();
    QJsonArray headerExtArray = headerExtValue.toArray();

    for (const QJsonValue value : headersArray) {
        QJsonArray entry_array = value.toArray();
        if (entry_array.size() > 2) {
            // Array[0] : header path
            // Array[2] : Location of the generated moc file for this header
            // if no source file includes it
            headers.insert(entry_array[0].toString(), entry_array[2].toString());
        }
    }

    sources.reserve(sourcesArray.size());
    for (const QJsonValue value : sourcesArray) {
        QJsonArray entry_array = value.toArray();
        if (entry_array.size() > 1) {
            sources.push_back(entry_array[0].toString());
        }
    }

    headerExts.reserve(headerExtArray.size());
    for (const QJsonValue value : headerExtArray) {
        headerExts.push_back(value.toString());
    }

    // MOC_INCLUDES holds the target's moc include paths, which are used to resolve a
    // moc include back to a header that is not next to the including source.
    // Prefer the per-config key when it is set.
    QJsonValue mocIncludesValue;
    if (!config.isEmpty())
        mocIncludesValue = rootObject.value("MOC_INCLUDES_"_L1 + config);
    if (!mocIncludesValue.isArray())
        mocIncludesValue = rootObject.value("MOC_INCLUDES"_L1);
    if (mocIncludesValue.isArray()) {
        const QJsonArray mocIncludesArray = mocIncludesValue.toArray();
        mocIncludePaths.reserve(mocIncludesArray.size());
        for (const QJsonValue value : mocIncludesArray)
            mocIncludePaths.push_back(value.toString());
    }

    return true;
}

struct ParseCacheEntry
{
    QStringList mocFiles;
    QStringList mocIncludes;
};

using ParseCacheMap = QMap<QString, ParseCacheEntry>;

static bool readParseCache(ParseCacheMap &entries, const QString &parseCacheFilePath)
{

    QFile file(parseCacheFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "Could not open: %s\n", qPrintable(parseCacheFilePath));
        return false;
    }

    QString source;
    QStringList mocEntries;
    QStringList mocIncludes;

    // File format
    // ....
    // header/source path N
    //  mmc:Q_OBJECT| mcc:Q_GADGET # This file has been mocked
    //  miu:moc_....cpp # Path of the moc.cpp file generated for the above file
    //  relative to TARGET_BINARY_DIR/TARGET_autgen/include directory. Not
    //  present for headers.
    //  mid: ....moc # Path of .moc file generated for the above file relative
    //  to TARGET_BINARY_DIR/TARGET_autogen/include directory.
    //  uic: UI related info, ignored
    //  mdp: Moc dependencies, ignored
    //  udp: UI dependencies, ignored
    // header/source path N + 1
    // ....

    QTextStream textStream(&file);
    const QString mmcKey = QString(" mmc:"_L1);
    const QString miuKey = QString(" miu:"_L1);
    const QString uicKey = QString(" uic:"_L1);
    const QString midKey = QString(" mid:"_L1);
    const QString mdpKey = QString(" mdp:"_L1);
    const QString udpKey = QString(" udp:"_L1);
    QString line;
    bool mmc_key_found = false;
    while (textStream.readLineInto(&line)) {
        if (!line.startsWith(u' ')) {
            if (!mocEntries.isEmpty() || mmc_key_found || !mocIncludes.isEmpty()) {
                entries.insert(source,
                               ParseCacheEntry { std::move(mocEntries), std::move(mocIncludes) });
                source.clear();
                mocEntries = {};
                mocIncludes = {};
                mmc_key_found = false;
            }
            source = line;
        } else if (line.startsWith(mmcKey)) {
            mmc_key_found = true;
        } else if (line.startsWith(miuKey)) {
            mocIncludes.push_back(line.right(line.size() - miuKey.size()));
        } else if (line.startsWith(midKey)) {
            mocEntries.push_back(line.right(line.size() - midKey.size()));
        } else if (line.startsWith(uicKey) || line.startsWith(mdpKey) || line.startsWith(udpKey)) {
            // nothing to do ignore
            continue;
        } else {
            fprintf(stderr, "Unhandled line entry \"%s\" in %s\n", qPrintable(line),
                    qPrintable(parseCacheFilePath));
            return false;
        }
    }

    // Check if last entry has any data left to processed
    if (!mocEntries.isEmpty() || !mocIncludes.isEmpty() || mmc_key_found) {
        entries.insert(source, ParseCacheEntry { std::move(mocEntries), std::move(mocIncludes) });
    }

    file.close();
    return true;
}

static bool writeJsonFiles(const QList<QString> &fileList, const QString &fileListFilePath,
                           const QString &timestampFilePath)
{
    QFile timestampFile(timestampFilePath);
    if (!timestampFile.open(QIODevice::ReadWrite)) {
        fprintf(stderr, "Could not open: %s\n", qPrintable(timestampFilePath));
        return false;
    }

    qint64 timestamp = std::numeric_limits<qint64>::min();
    if (timestampFile.size() == sizeof(timestamp))
        timestampFile.read(reinterpret_cast<char *>(&timestamp), sizeof(timestamp));

    const qint64 previousTimestamp = timestamp;

    // Check if any of the metatype json files produced by automoc is newer than the last file
    // processed by cmake_automoc parser
    for (const auto &jsonFile : fileList) {
        const qint64 jsonFileLastModified =
                QFileInfo(jsonFile).lastModified(QTimeZone::UTC).toMSecsSinceEpoch();
        if (jsonFileLastModified > timestamp) {
            timestamp = jsonFileLastModified;
        }
    }

    QByteArray newContent;
    for (const auto &jsonFile : fileList) {
        newContent += jsonFile.toUtf8();
        newContent += '\n';
    }

    bool needsRewrite = timestamp > previousTimestamp;
    if (!needsRewrite) {
        // Timestamps are unchanged, but the set of json files might have changed
        // (e.g. a Q_OBJECT macro was removed from a header). Compare the file lists.
        QFile existingFile(fileListFilePath);
        if (!existingFile.open(QIODevice::ReadOnly | QIODevice::Text)
            || existingFile.readAll() != newContent) {
            needsRewrite = true;
        }
    }

    if (needsRewrite) {
        QFile file(fileListFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            fprintf(stderr, "Could not open: %s\n", qPrintable(fileListFilePath));
            return false;
        }
        file.write(newContent);

        // Update the timestamp according the newest json file timestamp.
        timestampFile.resize(0);
        timestampFile.write(reinterpret_cast<char *>(&timestamp), sizeof(timestamp));
    }
    return true;
}

int main(int argc, char **argv)
{

    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt CMake Autogen parser tool"));

    parser.addHelpOption();
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    QCommandLineOption outputFileOption(QStringLiteral("output-file-path"));
    outputFileOption.setDescription(
            QStringLiteral("Output file where the meta type file list will be written."));
    outputFileOption.setValueName(QStringLiteral("output file"));
    parser.addOption(outputFileOption);

    QCommandLineOption cmakeAutogenCacheFileOption(QStringLiteral("cmake-autogen-cache-file"));
    cmakeAutogenCacheFileOption.setDescription(
            QStringLiteral("Location of the CMake AutoGen ParseCache.txt file."));
    cmakeAutogenCacheFileOption.setValueName(QStringLiteral("CMake AutoGen ParseCache.txt file"));
    parser.addOption(cmakeAutogenCacheFileOption);

    QCommandLineOption cmakeAutogenInfoFileOption(QStringLiteral("cmake-autogen-info-file"));
    cmakeAutogenInfoFileOption.setDescription(
            QStringLiteral("Location of the CMake AutoGen AutogenInfo.json file."));
    cmakeAutogenInfoFileOption.setValueName(QStringLiteral("CMake AutoGen AutogenInfo.json file"));
    parser.addOption(cmakeAutogenInfoFileOption);

    QCommandLineOption cmakeAutogenIncludeDirOption(
            QStringLiteral("cmake-autogen-include-dir-path"));
    cmakeAutogenIncludeDirOption.setDescription(
            QStringLiteral("Location of the CMake AutoGen include directory."));
    cmakeAutogenIncludeDirOption.setValueName(QStringLiteral("CMake AutoGen include directory"));
    parser.addOption(cmakeAutogenIncludeDirOption);

    QCommandLineOption isMultiConfigOption(
            QStringLiteral("cmake-multi-config"));
    isMultiConfigOption.setDescription(
            QStringLiteral("Set this option when using CMake with a multi-config generator"));
    parser.addOption(isMultiConfigOption);

    QCommandLineOption configOption(QStringLiteral("config"));
    configOption.setDescription(
            QStringLiteral("The active CMake configuration (e.g. Debug). Only meaningful "
                           "for multi-config generators"));
    configOption.setValueName(QStringLiteral("config"));
    parser.addOption(configOption);

    QCommandLineOption timestampFilePathOption(QStringLiteral("timestamp-file-path"));
    timestampFilePathOption.setDescription(
            QStringLiteral("The path to a timestamp file that determines whether the output"
                           " file needs to be updated."));
    timestampFilePathOption.setValueName(QStringLiteral("timestamp file"));
    parser.addOption(timestampFilePathOption);

    QCommandLineOption debugOption(QStringLiteral("debug"));
    debugOption.setDescription(
            QStringLiteral("Print verbose diagnostics to stderr."));
    parser.addOption(debugOption);

    QStringList arguments = QCoreApplication::arguments();
    parser.process(arguments);

    if (!parser.isSet(outputFileOption) || !parser.isSet(cmakeAutogenInfoFileOption)
        || !parser.isSet(cmakeAutogenCacheFileOption)
        || !parser.isSet(cmakeAutogenIncludeDirOption)) {
        parser.showHelp(1);
        return EXIT_FAILURE;
    }

    const bool debug = parser.isSet(debugOption);

    // Read source files from AutogenInfo.json
    AutoGenHeaderMap autoGenHeaders;
    AutoGenSourcesList autoGenSources;
    QStringList headerExtList;
    QStringList mocIncludePaths;
    const QString cmakeAutogenInfoFile = parser.value(cmakeAutogenInfoFileOption);
    const QString cmakeAutogenCacheFile = parser.value(cmakeAutogenCacheFileOption);
    const QString config = parser.value(configOption);

    if (debug)
        fprintf(stderr, "Parsing %s\n", qUtf8Printable(cmakeAutogenInfoFile));
    if (!readAutogenInfoJson(autoGenHeaders, autoGenSources, headerExtList, mocIncludePaths,
                             config, cmakeAutogenInfoFile)) {
        if (debug)
            fprintf(stderr, "Failed to read AutogenInfo.json file: %s\n",
                    qUtf8Printable(cmakeAutogenInfoFile));
        return EXIT_FAILURE;
    }

    if (debug)
        fprintf(stderr, "Parsing %s\n", qUtf8Printable(cmakeAutogenCacheFile));

    ParseCacheMap parseCacheEntries;
    if (!readParseCache(parseCacheEntries, cmakeAutogenCacheFile)) {
        if (debug)
            fprintf(stderr, "Failed to read parse cache file: %s\n",
                    qUtf8Printable(cmakeAutogenCacheFile));
        return EXIT_FAILURE;
    }

    const QString cmakeIncludeDir = parser.value(cmakeAutogenIncludeDirOption);

    if (debug) {
        fprintf(stderr, "AutoGen include dir: %s\n", qUtf8Printable(cmakeIncludeDir));
        fprintf(stderr, "%lld header(s) from AutogenInfo.json (path -> moc file location if not "
                        "included by a source):\n",
                static_cast<long long>(autoGenHeaders.size()));
        for (auto it = autoGenHeaders.cbegin(); it != autoGenHeaders.cend(); ++it)
            fprintf(stderr, "    %s -> %s\n", qUtf8Printable(it.key()), qUtf8Printable(it.value()));
        fprintf(stderr, "%lld source(s) from AutogenInfo.json:\n",
                static_cast<long long>(autoGenSources.size()));
        for (const auto &source : autoGenSources)
            fprintf(stderr, "    %s\n", qUtf8Printable(source));
        fprintf(stderr, "config: %s\n",
                config.isEmpty() ? "<none>" : qUtf8Printable(config));
        fprintf(stderr, "%lld moc include path(s) from AutogenInfo.json:\n",
                static_cast<long long>(mocIncludePaths.size()));
        for (const auto &includePath : mocIncludePaths)
            fprintf(stderr, "    %s\n", qUtf8Printable(includePath));
    }

    // Algorithm description
    // 1) For each source from the AutoGenSources list that has a parse cache entry:
    // - for each #include "<base>.moc" file in 'mocFiles' add the <base>.moc.json to the output
    //   because this is the source moc file
    // - for each #include "moc_<base>.cpp" file in 'mocIncludes', search for a header with the
    //   same base name next to the source file or the moc include dirs.
    //   If found, add moc_<base>.cpp.json to the output, and remove the <base>.h header from
    //   AutoGenHeaders so it does not get a standalone moc_<base>.cpp.json entry in step 2.
    // 2) For every header still left in AutoGenHeaders, add the .json file for its standalone moc
    //   file, using the location recorded in AutogenInfo.json.

    QList<QString> jsonFileList;
    QDir dir(cmakeIncludeDir);
    jsonFileList.reserve(autoGenSources.size());

    // 1) Process sources
    if (debug)
        fprintf(stderr, "Processing %lld source(s)\n",
                static_cast<long long>(autoGenSources.size()));
    for (const auto &source : autoGenSources) {
        auto it = parseCacheEntries.find(source);
        if (it == parseCacheEntries.end()) {
            if (debug)
                fprintf(stderr, "  source %s: no parse cache entry, skipping\n",
                        qUtf8Printable(source));
            continue;
        }

        const QFileInfo fileInfo(source);
        if (debug)
            fprintf(stderr, "  source: %s\n", qUtf8Printable(source));

        // Add source-file based <base>.moc files
        for (const auto &mocFile : it.value().mocFiles) {
            const QString jsonPath = dir.filePath(mocFile) + ".json"_L1;
            if (debug)
                fprintf(stderr, "    mid: %s -> adding %s\n", qUtf8Printable(mocFile),
                        qUtf8Printable(jsonPath));
            jsonFileList.push_back(jsonPath);
        }
        // Add header-file based moc_<base>.cpp files
        for (const auto &mocFile : it.value().mocIncludes) {
            const QString jsonPath = dir.filePath(mocFile) + ".json"_L1;
            if (debug)
                fprintf(stderr, "    miu: %s -> adding %s\n", qUtf8Printable(mocFile),
                        qUtf8Printable(jsonPath));
            jsonFileList.push_back(jsonPath);

            // Locate the header this moc include was generated from and remove it from
            // processing, so it does not also get a standalone moc entry in step 2.
            // This mirrors CMake AUTOMOC's cmQtAutoMocUicT::JobEvalCacheMocT::FindIncludedHeader.
            // Search the vicinity of the source first, then each moc include path in order.
            // Example:
            // sourceFile:         "sub/foo.cpp"
            // sourceFileDir:      "sub"
            // mocFile:            "sub/moc_foo.cpp"
            // mocIncludeFileName: "moc_foo.cpp"
            // headerBaseName:     "foo"
            // mocFileBaseDir:     "sub"
            // headerRelBase:      "sub/foo"
            // vicinity candidate: "sub/sub/foo.h" yes, double "sub", this is what CMake does too
            // include  candidate: "<I>/sub/foo.h" for each moc include path I
            constexpr int mocPrefixLength = 4; // length of "moc_"
            const QString mocIncludeFileName = QFileInfo(mocFile).fileName();
            const QString headerBaseName =
                    QFileInfo(mocIncludeFileName.right(
                                mocIncludeFileName.size() - mocPrefixLength))
                            .completeBaseName();
            const QString mocFileBaseDir = QFileInfo(mocFile).path();
            const QString sourceFileDir = fileInfo.path();

            // The header path relative to the directory it is searched in, e.g. "sub/foo" for
            // #include "sub/moc_foo.cpp".
            const QString headerRelBase = mocFileBaseDir + u'/' + headerBaseName;

            auto findHeaderIn = [&](const QString &headerSearchDir) {
                for (const auto &headerExtension : headerExtList) {
                    const QString candidateHeaderPath = QDir::cleanPath(
                            headerSearchDir + u'/' + headerRelBase + u'.' + headerExtension);
                    auto found = autoGenHeaders.find(candidateHeaderPath);
                    if (found != autoGenHeaders.end())
                        return found;
                }
                return autoGenHeaders.end();
            };

            // Look in the vicinity of the source file.
            auto matchedHeader = findHeaderIn(sourceFileDir);
            QString matchedDir = sourceFileDir;
            QString matchedLabel = u"vicinity of source"_s;

            // Look in each moc include path.
            if (matchedHeader == autoGenHeaders.end()) {
                for (const QString &includePath : mocIncludePaths) {
                    matchedHeader = findHeaderIn(includePath);
                    if (matchedHeader != autoGenHeaders.end()) {
                        matchedDir = includePath;
                        matchedLabel = u"moc include path"_s;
                        break;
                    }
                }
            }

            if (matchedHeader != autoGenHeaders.end()) {
                if (debug)
                    fprintf(stderr,
                            "    removed header %s from processing (matched moc include"
                            " '%s' via %s %s)\n",
                            qUtf8Printable(matchedHeader.key()), qUtf8Printable(mocFile),
                            qUtf8Printable(matchedLabel), qUtf8Printable(matchedDir));
                autoGenHeaders.erase(matchedHeader);
            } else if (debug) {
                fprintf(stderr,
                        "    no header matched moc include '%s' (searched as %s.<ext>):\n",
                        qUtf8Printable(mocFile), qUtf8Printable(headerRelBase));
                fprintf(stderr, "         vicinity dir: %s\n", qUtf8Printable(sourceFileDir));
                if (!mocIncludePaths.isEmpty())
                    fprintf(stderr, "         nor in the moc include dirs\n");
                fprintf(stderr, "         continuing\n");
            }
        }
    }

    // 2) Process headers
    if (debug)
        fprintf(stderr, "Processing %lld remaining header(s)\n",
                static_cast<long long>(autoGenHeaders.size()));
    const bool isMultiConfig = parser.isSet(isMultiConfigOption);
    for (auto mapIt = autoGenHeaders.begin(); mapIt != autoGenHeaders.end(); ++mapIt) {
        auto it = parseCacheEntries.find(mapIt.key());
        if (it == parseCacheEntries.end()) {
            if (debug)
                fprintf(stderr, "  header %s: no parse cache entry, skipping\n",
                        qUtf8Printable(mapIt.key()));
            continue;
        }
        const QString pathPrefix = !isMultiConfig
            ? QStringLiteral("../")
            : QString();
        const QString jsonPath = dir.filePath(pathPrefix + mapIt.value() + ".json"_L1);
        if (debug)
            fprintf(stderr, "  header %s:\n    adding %s\n", qUtf8Printable(mapIt.key()),
                    qUtf8Printable(jsonPath));
        jsonFileList.push_back(jsonPath);
    }

    // Sort for consistent checks across runs
    jsonFileList.sort();

    if (debug) {
        fprintf(stderr, "Final metatypes json file list (%lld entries):\n",
                static_cast<long long>(jsonFileList.size()));
        for (const auto &jsonFile : jsonFileList)
            fprintf(stderr, "    %s\n", qUtf8Printable(jsonFile));
    }

    // Read Previous file list (if any)
    if (!writeJsonFiles(jsonFileList, parser.value(outputFileOption),
                        parser.value(timestampFilePathOption))) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

QT_END_NAMESPACE
