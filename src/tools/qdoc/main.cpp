/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  main.cpp
*/

#include <qglobal.h>
#include <stdlib.h>
#include "codemarker.h"
#include "codeparser.h"
#include "config.h"
#include "cppcodemarker.h"
#include "cppcodeparser.h"
#include "ditaxmlgenerator.h"
#include "doc.h"
#include "htmlgenerator.h"
#include "plaincodemarker.h"
#include "puredocparser.h"
#include "tokenizer.h"
#include "tree.h"

#include "jscodemarker.h"
#include "qmlcodemarker.h"
#include "qmlcodeparser.h"

#include <qdatetime.h>
#include <qdebug.h>

#include "qtranslator.h"
#ifndef QT_BOOTSTRAPPED
#  include "qcoreapplication.h"
#endif

QT_BEGIN_NAMESPACE

/*
  The default indent for code is 4.
  The default value for false is 0.
  The default supported file extensions are cpp, h, qdoc and qml.
  The default language is c++.
  The default output format is html.
  The default tab size is 8.
  And those are all the default values for configuration variables.
 */
static const struct {
    const char *key;
    const char *value;
} defaults[] = {
    { CONFIG_CODEINDENT, "4" },
    { CONFIG_FALSEHOODS, "0" },
    { CONFIG_FILEEXTENSIONS, "*.cpp *.h *.qdoc *.qml"},
    { CONFIG_LANGUAGE, "Cpp" },
    { CONFIG_OUTPUTFORMATS, "HTML" },
    { CONFIG_TABSIZE, "8" },
    { 0, 0 }
};

static bool highlighting = false;
static bool showInternal = false;
static bool obsoleteLinks = false;
static QStringList defines;
static QStringList indexDirs;
static QHash<QString, Tree *> trees;

/*!
  Print the help message to \c stdout.
 */
static void printHelp()
{
    Location::information(tr("Usage: qdoc [options] file1.qdocconf ...\n"
                             "Options:\n"
                             "    -D<name>       "
                             "Define <name> as a macro while parsing sources\n"
                             "    -help          "
                             "Display this information and exit\n"
                             "    -highlighting  "
                             "Turn on syntax highlighting (makes qdoc run slower)\n"
                             "    -indexdir      "
                             "Specify a directory where QDoc should search for indices to link to\n"
                             "    -installdir    "
                             "Specify the directory where the output will be after running \"make install\"\n"
                             "    -no-examples   "
                             "Do not generate documentation for examples\n"
                             "    -obsoletelinks "
                             "Report links from obsolete items to non-obsolete items\n"
                             "    -outputdir     "
                             "Specify output directory, overrides setting in qdocconf file\n"
                             "    -outputformat  "
                             "Specify output format, overrides setting in qdocconf file\n"
                             "    -showinternal  "
                             "Include content marked internal\n"
                             "    -version       "
                             "Display version of qdoc and exit\n") );
}

/*!
  Prints the qdoc version number to stdout.
 */
static void printVersion()
{
    QString s = tr("qdoc version %1").arg(QT_VERSION_STR);
    Location::information(s);
}

/*!
  Processes the qdoc config file \a fileName. This is the
  controller for all of qdoc.
 */
static void processQdocconfFile(const QString &fileName)
{
#ifndef QT_NO_TRANSLATION
    QList<QTranslator *> translators;
#endif

    /*
      The Config instance represents the configuration data for qdoc.
      All the other classes are initialized with the config. Here we
      initialize the configuration with some default values.
     */
    Config config(tr("qdoc"));
    int i = 0;
    while (defaults[i].key) {
        config.setStringList(defaults[i].key,
                             QStringList() << defaults[i].value);
        ++i;
    }
    config.setStringList(CONFIG_SYNTAXHIGHLIGHTING, QStringList(highlighting ? "true" : "false"));
    config.setStringList(CONFIG_SHOWINTERNAL,
                         QStringList(showInternal ? "true" : "false"));
    config.setStringList(CONFIG_OBSOLETELINKS,
                         QStringList(obsoleteLinks ? "true" : "false"));

    /*
      With the default configuration values in place, load
      the qdoc configuration file. Note that the configuration
      file may include other configuration files.

      The Location class keeps track of the current location
      in the file being processed, mainly for error reporting
      purposes.
     */
    Location::initialize(config);
    config.load(fileName);

    /*
      Add the defines to the configuration variables.
     */
    QStringList defs = defines + config.getStringList(CONFIG_DEFINES);
    config.setStringList(CONFIG_DEFINES,defs);
    Location::terminate();

    QString prevCurrentDir = QDir::currentPath();
    QString dir = QFileInfo(fileName).path();
    if (!dir.isEmpty())
        QDir::setCurrent(dir);

    /*
      Initialize all the classes and data structures with the
      qdoc configuration.
     */
    Location::initialize(config);
    Tokenizer::initialize(config);
    Doc::initialize(config);
    CodeMarker::initialize(config);
    CodeParser::initialize(config);
    Generator::initialize(config);

#ifndef QT_NO_TRANSLATION
    /*
      Load the language translators, if the configuration specifies any.
     */
    QStringList fileNames = config.getStringList(CONFIG_TRANSLATORS);
    QStringList::Iterator fn = fileNames.begin();
    while (fn != fileNames.end()) {
        QTranslator *translator = new QTranslator(0);
        if (!translator->load(*fn))
            config.lastLocation().error(tr("Cannot load translator '%1'")
                                        .arg(*fn));
        QCoreApplication::instance()->installTranslator(translator);
        translators.append(translator);
        ++fn;
    }
#endif

    //QSet<QString> outputLanguages = config.getStringSet(CONFIG_OUTPUTLANGUAGES);

    /*
      Get the source language (Cpp) from the configuration
      and the location in the configuration file where the
      source language was set.
     */
    QString lang = config.getString(CONFIG_LANGUAGE);
    Location langLocation = config.lastLocation();

    /*
      Initialize the tree where all the parsed sources will be stored.
      The tree gets built as the source files are parsed, and then the
      documentation output is generated by traversing the tree.
     */
    Tree *tree = new Tree;
    tree->setVersion(config.getString(CONFIG_VERSION));

    /*
      By default, the only output format is HTML.
     */
    QSet<QString> outputFormats = config.getOutputFormats();
    Location outputFormatsLocation = config.lastLocation();

    /*
      Read some XML indexes containing definitions from other documentation sets.
     */
    QStringList indexFiles = config.getStringList(CONFIG_INDEXES);

    QStringList dependModules = config.getStringList(CONFIG_DEPENDS);

    if (dependModules.size() > 0) {
        if (indexDirs.size() > 0) {
            for (int i = 0; i < dependModules.size(); i++) {
                QMultiMap<uint, QFileInfo> foundIndices;
                for (int j = 0; j < indexDirs.size(); j++) {
                    QString fileToLookFor = indexDirs[j] + QLatin1Char('/') + dependModules[i] +
                            QLatin1Char('/') + dependModules[i] + QLatin1String(".index");
                    if (QFile::exists(fileToLookFor)) {
                        QFileInfo tempFileInfo(fileToLookFor);
                        foundIndices.insert(tempFileInfo.lastModified().toTime_t(), tempFileInfo);
                    }
                }
                if (foundIndices.size() > 1) {
                    /*
                        QDoc should always use the last entry in the multimap when there are
                        multiple index files for a module, since the last modified file has the
                        highest UNIX timestamp.
                    */
                    qDebug() << "Multiple indices found for dependency:" << dependModules[i];
                    qDebug() << "Using" << foundIndices.value(
                                    foundIndices.keys()[foundIndices.size() - 1]).absoluteFilePath()
                            << "as index for" << dependModules[i];
                    indexFiles << foundIndices.value(
                                      foundIndices.keys()[foundIndices.size() - 1]).absoluteFilePath();
                }
                else if (foundIndices.size() == 1) {
                    indexFiles << foundIndices.value(foundIndices.keys()[0]).absoluteFilePath();
                }
                else {
                    qDebug() << "No indices for" << dependModules[i] <<
                                "could be found in the specified index directories.";
                }
            }
        }
        else {
            qDebug() << "Dependant modules specified, but not index directories were set."
                     << "There will probably be errors for missing links.";
        }
    }
    tree->readIndexes(indexFiles);

    QSet<QString> excludedDirs;
    QSet<QString> excludedFiles;
    QSet<QString> headers;
    QSet<QString> sources;
    QStringList headerList;
    QStringList sourceList;
    QStringList excludedDirsList;
    QStringList excludedFilesList;

    excludedDirsList = config.getCleanPathList(CONFIG_EXCLUDEDIRS);
    foreach (const QString &excludeDir, excludedDirsList) {
        QString p = QDir::fromNativeSeparators(excludeDir);
        excludedDirs.insert(p);
    }

    excludedFilesList = config.getCleanPathList(CONFIG_EXCLUDEFILES);
    foreach (const QString& excludeFile, excludedFilesList) {
        QString p = QDir::fromNativeSeparators(excludeFile);
        excludedFiles.insert(p);
    }

    headerList = config.getAllFiles(CONFIG_HEADERS,CONFIG_HEADERDIRS,excludedDirs,excludedFiles);
    headers = QSet<QString>::fromList(headerList);

    sourceList = config.getAllFiles(CONFIG_SOURCES,CONFIG_SOURCEDIRS,excludedDirs,excludedFiles);
    sources = QSet<QString>::fromList(sourceList);

    /*
      Parse each header file in the set using the appropriate parser and add it
      to the big tree.
     */
    QSet<CodeParser *> usedParsers;

    QSet<QString>::ConstIterator h = headers.begin();
    while (h != headers.end()) {
        CodeParser *codeParser = CodeParser::parserForHeaderFile(*h);
        if (codeParser) {
            codeParser->parseHeaderFile(config.location(), *h, tree);
            usedParsers.insert(codeParser);
        }
        ++h;
    }

    foreach (CodeParser *codeParser, usedParsers)
        codeParser->doneParsingHeaderFiles(tree);

    usedParsers.clear();
    /*
      Parse each source text file in the set using the appropriate parser and
      add it to the big tree.
     */
    QSet<QString>::ConstIterator s = sources.begin();
    while (s != sources.end()) {
        CodeParser *codeParser = CodeParser::parserForSourceFile(*s);
        if (codeParser) {
            codeParser->parseSourceFile(config.location(), *s, tree);
            usedParsers.insert(codeParser);
        }
        ++s;
    }

    foreach (CodeParser *codeParser, usedParsers)
        codeParser->doneParsingSourceFiles(tree);

    /*
      Now the big tree has been built from all the header and
      source files. Resolve all the class names, function names,
      targets, URLs, links, and other stuff that needs resolving.
     */
    tree->resolveGroups();
    tree->resolveTargets(tree->root());
    tree->resolveCppToQmlLinks();
    tree->resolveQmlInheritance();

    /*
      The tree is built and all the stuff that needed resolving
      has been resolved. Now traverse the tree and generate the
      documentation output. More than one output format can be
      requested. The tree is traversed for each one.
     */
    QSet<QString>::ConstIterator of = outputFormats.begin();
    while (of != outputFormats.end()) {
        Generator* generator = Generator::generatorForFormat(*of);
        if (generator == 0)
            outputFormatsLocation.fatal(tr("Unknown output format '%1'").arg(*of));
        generator->generateTree(tree);
        ++of;
    }

    /*
      Generate the XML tag file, if it was requested.
     */
    QString tagFile = config.getString(CONFIG_TAGFILE);
    if (!tagFile.isEmpty()) {
        tree->generateTagFile(tagFile);
    }

    tree->setVersion(QString());
    Generator::terminate();
    CodeParser::terminate();
    CodeMarker::terminate();
    Doc::terminate();
    Tokenizer::terminate();
    Location::terminate();
    QDir::setCurrent(prevCurrentDir);

#ifndef QT_NO_TRANSLATION
    qDeleteAll(translators);
#endif
#ifdef DEBUG_SHUTDOWN_CRASH
    qDebug() << "main(): Delete tree";
#endif
    delete tree;
#ifdef DEBUG_SHUTDOWN_CRASH
    qDebug() << "main(): Tree deleted";
#endif
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    QT_USE_NAMESPACE

#ifndef QT_BOOTSTRAPPED
    QCoreApplication app(argc, argv);
#endif

    /*
      Create code parsers for the languages to be parsed,
      and create a tree for C++.
     */
    CppCodeParser cppParser;
    QmlCodeParser qmlParser;
    PureDocParser docParser;

    /*
      Create code markers for plain text, C++,
      javascript, and QML.
     */
    PlainCodeMarker plainMarker;
    CppCodeMarker cppMarker;
    JsCodeMarker jsMarker;
    QmlCodeMarker qmlMarker;

    HtmlGenerator htmlGenerator;
    DitaXmlGenerator ditaxmlGenerator;

    QStringList qdocFiles;
    QString opt;
    int i = 1;

    while (i < argc) {
        opt = argv[i++];

        if (opt == "-help") {
            printHelp();
            return EXIT_SUCCESS;
        }
        else if (opt == "-version") {
            printVersion();
            return EXIT_SUCCESS;
        }
        else if (opt == "--") {
            while (i < argc)
                qdocFiles.append(argv[i++]);
        }
        else if (opt.startsWith("-D")) {
            QString define = opt.mid(2);
            defines += define;
        }
        else if (opt == "-highlighting") {
            highlighting = true;
        }
        else if (opt == "-showinternal") {
            showInternal = true;
        }
        else if (opt == "-no-examples") {
            Config::generateExamples = false;
        }
        else if (opt == "-indexdir") {
            if (QFile::exists(argv[i])) {
                indexDirs += argv[i];
            }
            else {
                qDebug() << "Cannot find index directory" << argv[i];
                return EXIT_FAILURE;
            }
            i++;
        }
        else if (opt == "-installdir") {
            Config::installDir = argv[i];
            i++;
        }
        else if (opt == "-obsoletelinks") {
            obsoleteLinks = true;
        }
        else if (opt == "-outputdir") {
            Config::overrideOutputDir = argv[i];
            i++;
        }
        else if (opt == "-outputformat") {
            Config::overrideOutputFormats.insert(argv[i]);
            i++;
        }
        else {
            qdocFiles.append(opt);
        }
    }

    if (qdocFiles.isEmpty()) {
        printHelp();
        return EXIT_FAILURE;
    }

    /*
      Main loop.
     */
    foreach (QString qf, qdocFiles) {
        //qDebug() << "PROCESSING:" << qf;
        processQdocconfFile(qf);
    }

    qDeleteAll(trees);
    return EXIT_SUCCESS;
}

