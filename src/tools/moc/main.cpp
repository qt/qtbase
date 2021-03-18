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

#include "preprocessor.h"
#include "moc.h"
#include "outputrevision.h"
#include "collectjson.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <qcoreapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qscopedpointer.h>

QT_BEGIN_NAMESPACE

/*
    This function looks at two file names and returns the name of the
    infile with a path relative to outfile.

    Examples:

        /tmp/abc, /tmp/bcd -> abc
        xyz/a/bc, xyz/b/ac -> ../a/bc
        /tmp/abc, xyz/klm -> /tmp/abc
 */

static QByteArray combinePath(const QString &infile, const QString &outfile)
{
    QFileInfo inFileInfo(QDir::current(), infile);
    QFileInfo outFileInfo(QDir::current(), outfile);
    const QByteArray relativePath = QFile::encodeName(outFileInfo.dir().relativeFilePath(inFileInfo.filePath()));
#ifdef Q_OS_WIN
    // It's a system limitation.
    // It depends on the Win API function which is used by the program to open files.
    // cl apparently uses the functions that have the MAX_PATH limitation.
    if (outFileInfo.dir().absolutePath().length() + relativePath.length() + 1 >= 260)
        return QFile::encodeName(inFileInfo.absoluteFilePath());
#endif
    return relativePath;
}


void error(const char *msg = "Invalid argument")
{
    if (msg)
        fprintf(stderr, "moc: %s\n", msg);
}

struct ScopedPointerFileCloser
{
    static inline void cleanup(FILE *handle) { if (handle) fclose(handle); }
};

static inline bool hasNext(const Symbols &symbols, int i)
{ return (i < symbols.size()); }

static inline const Symbol &next(const Symbols &symbols, int &i)
{ return symbols.at(i++); }


QByteArray composePreprocessorOutput(const Symbols &symbols) {
    QByteArray output;
    int lineNum = 1;
    Token last = PP_NOTOKEN;
    Token secondlast = last;
    int i = 0;
    while (hasNext(symbols, i)) {
        Symbol sym = next(symbols, i);
        switch (sym.token) {
        case PP_NEWLINE:
        case PP_WHITESPACE:
            if (last != PP_WHITESPACE) {
                secondlast = last;
                last = PP_WHITESPACE;
                output += ' ';
            }
            continue;
        case PP_STRING_LITERAL:
            if (last == PP_STRING_LITERAL)
                output.chop(1);
            else if (secondlast == PP_STRING_LITERAL && last == PP_WHITESPACE)
                output.chop(2);
            else
                break;
            output += sym.lexem().mid(1);
            secondlast = last;
            last = PP_STRING_LITERAL;
            continue;
        case MOC_INCLUDE_BEGIN:
            lineNum = 0;
            continue;
        case MOC_INCLUDE_END:
            lineNum = sym.lineNum;
            continue;
        default:
            break;
        }
        secondlast = last;
        last = sym.token;

        const int padding = sym.lineNum - lineNum;
        if (padding > 0) {
            output.resize(output.size() + padding);
            memset(output.data() + output.size() - padding, '\n', padding);
            lineNum = sym.lineNum;
        }

        output += sym.lexem();
    }

    return output;
}

static QStringList argumentsFromCommandLineAndFile(const QStringList &arguments)
{
    QStringList allArguments;
    allArguments.reserve(arguments.size());
    for (const QString &argument : arguments) {
        // "@file" doesn't start with a '-' so we can't use QCommandLineParser for it
        if (argument.startsWith(QLatin1Char('@'))) {
            QString optionsFile = argument;
            optionsFile.remove(0, 1);
            if (optionsFile.isEmpty()) {
                error("The @ option requires an input file");
                return QStringList();
            }
            QFile f(optionsFile);
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                error("Cannot open options file specified with @");
                return QStringList();
            }
            while (!f.atEnd()) {
                QString line = QString::fromLocal8Bit(f.readLine().trimmed());
                if (!line.isEmpty())
                    allArguments << line;
            }
        } else {
            allArguments << argument;
        }
    }
    return allArguments;
}

// Escape characters in given path. Dependency paths are Make-style, not NMake/Jom style.
// The paths can also be consumed by Ninja.
// "$" replaced by "$$"
// "#" replaced by "\#"
// " " replaced by "\ "
// "\#" replaced by "\\#"
// "\ " replaced by "\\\ "
//
// The escape rules are according to what clang / llvm escapes when generating a Make-style
// dependency file.
// Is a template function, because input param can be either a QString or a QByteArray.
template <typename T> struct CharType;
template <> struct CharType<QString> { using type = QLatin1Char; };
template <> struct CharType<QByteArray> { using type = char; };
template <typename StringType>
StringType escapeDependencyPath(const StringType &path)
{
    using CT = typename CharType<StringType>::type;
    StringType escapedPath;
    int size = path.size();
    escapedPath.reserve(size);
    for (int i = 0; i < size; ++i) {
        if (path[i] == CT('$')) {
            escapedPath.append(CT('$'));
        } else if (path[i] == CT('#')) {
            escapedPath.append(CT('\\'));
        } else if (path[i] == CT(' ')) {
            escapedPath.append(CT('\\'));
            int backwards_it = i - 1;
            while (backwards_it > 0 && path[backwards_it] == CT('\\')) {
                escapedPath.append(CT('\\'));
                --backwards_it;
            }
        }
        escapedPath.append(path[i]);
    }
    return escapedPath;
}

QByteArray escapeAndEncodeDependencyPath(const QString &path)
{
    return QFile::encodeName(escapeDependencyPath(path));
}

int runMoc(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationVersion(QString::fromLatin1(QT_VERSION_STR));

    bool autoInclude = true;
    bool defaultInclude = true;
    Preprocessor pp;
    Moc moc;
    pp.macros["Q_MOC_RUN"];
    pp.macros["__cplusplus"];

    // Don't stumble over GCC extensions
    Macro dummyVariadicFunctionMacro;
    dummyVariadicFunctionMacro.isFunction = true;
    dummyVariadicFunctionMacro.isVariadic = true;
    dummyVariadicFunctionMacro.arguments += Symbol(0, PP_IDENTIFIER, "__VA_ARGS__");
    pp.macros["__attribute__"] = dummyVariadicFunctionMacro;
    pp.macros["__declspec"] = dummyVariadicFunctionMacro;

    QString filename;
    QString output;
    QFile in;
    FILE *out = 0;

    // Note that moc isn't translated.
    // If you use this code as an example for a translated app, make sure to translate the strings.
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt Meta Object Compiler version %1 (Qt %2)")
                                     .arg(mocOutputRevision).arg(QString::fromLatin1(QT_VERSION_STR)));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    QCommandLineOption outputOption(QStringLiteral("o"));
    outputOption.setDescription(QStringLiteral("Write output to file rather than stdout."));
    outputOption.setValueName(QStringLiteral("file"));
    outputOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(outputOption);

    QCommandLineOption includePathOption(QStringLiteral("I"));
    includePathOption.setDescription(QStringLiteral("Add dir to the include path for header files."));
    includePathOption.setValueName(QStringLiteral("dir"));
    includePathOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(includePathOption);

    QCommandLineOption macFrameworkOption(QStringLiteral("F"));
    macFrameworkOption.setDescription(QStringLiteral("Add Mac framework to the include path for header files."));
    macFrameworkOption.setValueName(QStringLiteral("framework"));
    macFrameworkOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(macFrameworkOption);

    QCommandLineOption preprocessOption(QStringLiteral("E"));
    preprocessOption.setDescription(QStringLiteral("Preprocess only; do not generate meta object code."));
    parser.addOption(preprocessOption);

    QCommandLineOption defineOption(QStringLiteral("D"));
    defineOption.setDescription(QStringLiteral("Define macro, with optional definition."));
    defineOption.setValueName(QStringLiteral("macro[=def]"));
    defineOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(defineOption);

    QCommandLineOption undefineOption(QStringLiteral("U"));
    undefineOption.setDescription(QStringLiteral("Undefine macro."));
    undefineOption.setValueName(QStringLiteral("macro"));
    undefineOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(undefineOption);

    QCommandLineOption metadataOption(QStringLiteral("M"));
    metadataOption.setDescription(QStringLiteral("Add key/value pair to plugin meta data"));
    metadataOption.setValueName(QStringLiteral("key=value"));
    metadataOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(metadataOption);

    QCommandLineOption compilerFlavorOption(QStringLiteral("compiler-flavor"));
    compilerFlavorOption.setDescription(QStringLiteral("Set the compiler flavor: either \"msvc\" or \"unix\"."));
    compilerFlavorOption.setValueName(QStringLiteral("flavor"));
    parser.addOption(compilerFlavorOption);

    QCommandLineOption noIncludeOption(QStringLiteral("i"));
    noIncludeOption.setDescription(QStringLiteral("Do not generate an #include statement."));
    parser.addOption(noIncludeOption);

    QCommandLineOption pathPrefixOption(QStringLiteral("p"));
    pathPrefixOption.setDescription(QStringLiteral("Path prefix for included file."));
    pathPrefixOption.setValueName(QStringLiteral("path"));
    pathPrefixOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(pathPrefixOption);

    QCommandLineOption forceIncludeOption(QStringLiteral("f"));
    forceIncludeOption.setDescription(QStringLiteral("Force #include <file> (overwrite default)."));
    forceIncludeOption.setValueName(QStringLiteral("file"));
    forceIncludeOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(forceIncludeOption);

    QCommandLineOption prependIncludeOption(QStringLiteral("b"));
    prependIncludeOption.setDescription(QStringLiteral("Prepend #include <file> (preserve default include)."));
    prependIncludeOption.setValueName(QStringLiteral("file"));
    prependIncludeOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(prependIncludeOption);

    QCommandLineOption includeOption(QStringLiteral("include"));
    includeOption.setDescription(QStringLiteral("Parse <file> as an #include before the main source(s)."));
    includeOption.setValueName(QStringLiteral("file"));
    parser.addOption(includeOption);

    QCommandLineOption noNotesWarningsCompatOption(QStringLiteral("n"));
    noNotesWarningsCompatOption.setDescription(QStringLiteral("Do not display notes (-nn) or warnings (-nw). Compatibility option."));
    noNotesWarningsCompatOption.setValueName(QStringLiteral("which"));
    noNotesWarningsCompatOption.setFlags(QCommandLineOption::ShortOptionStyle);
    parser.addOption(noNotesWarningsCompatOption);

    QCommandLineOption noNotesOption(QStringLiteral("no-notes"));
    noNotesOption.setDescription(QStringLiteral("Do not display notes."));
    parser.addOption(noNotesOption);

    QCommandLineOption noWarningsOption(QStringLiteral("no-warnings"));
    noWarningsOption.setDescription(QStringLiteral("Do not display warnings (implies --no-notes)."));
    parser.addOption(noWarningsOption);

    QCommandLineOption ignoreConflictsOption(QStringLiteral("ignore-option-clashes"));
    ignoreConflictsOption.setDescription(QStringLiteral("Ignore all options that conflict with compilers, like -pthread conflicting with moc's -p option."));
    parser.addOption(ignoreConflictsOption);

    QCommandLineOption jsonOption(QStringLiteral("output-json"));
    jsonOption.setDescription(QStringLiteral("In addition to generating C++ code, create a machine-readable JSON file in a file that matches the output file and an extra .json extension."));
    parser.addOption(jsonOption);

    QCommandLineOption collectOption(QStringLiteral("collect-json"));
    collectOption.setDescription(QStringLiteral("Instead of processing C++ code, collect previously generated JSON output into a single file."));
    parser.addOption(collectOption);

    QCommandLineOption depFileOption(QStringLiteral("output-dep-file"));
    depFileOption.setDescription(
                QStringLiteral("Output a Make-style dep file for build system consumption."));
    parser.addOption(depFileOption);

    QCommandLineOption depFilePathOption(QStringLiteral("dep-file-path"));
    depFilePathOption.setDescription(QStringLiteral("Path where to write the dep file."));
    depFilePathOption.setValueName(QStringLiteral("file"));
    parser.addOption(depFilePathOption);

    QCommandLineOption depFileRuleNameOption(QStringLiteral("dep-file-rule-name"));
    depFileRuleNameOption.setDescription(
                QStringLiteral("The rule name (first line) of the dep file."));
    depFileRuleNameOption.setValueName(QStringLiteral("rule name"));
    parser.addOption(depFileRuleNameOption);

    parser.addPositionalArgument(QStringLiteral("[header-file]"),
            QStringLiteral("Header file to read from, otherwise stdin."));
    parser.addPositionalArgument(QStringLiteral("[@option-file]"),
            QStringLiteral("Read additional options from option-file."));
    parser.addPositionalArgument(QStringLiteral("[MOC generated json file]"),
                                 QStringLiteral("MOC generated json output"));

    const QStringList arguments = argumentsFromCommandLineAndFile(app.arguments());
    if (arguments.isEmpty())
        return 1;

    parser.process(arguments);

    const QStringList files = parser.positionalArguments();
    output = parser.value(outputOption);
    if (parser.isSet(collectOption))
        return collectJson(files, output);

    if (files.count() > 1) {
        error(qPrintable(QLatin1String("Too many input files specified: '") + files.join(QLatin1String("' '")) + QLatin1Char('\'')));
        parser.showHelp(1);
    } else if (!files.isEmpty()) {
        filename = files.first();
    }

    const bool ignoreConflictingOptions = parser.isSet(ignoreConflictsOption);
    pp.preprocessOnly = parser.isSet(preprocessOption);
    if (parser.isSet(noIncludeOption)) {
        moc.noInclude = true;
        autoInclude = false;
    }
    if (!ignoreConflictingOptions) {
        if (parser.isSet(forceIncludeOption)) {
            moc.noInclude = false;
            autoInclude = false;
            const auto forceIncludes = parser.values(forceIncludeOption);
            for (const QString &include : forceIncludes) {
                moc.includeFiles.append(QFile::encodeName(include));
                defaultInclude = false;
             }
        }
        const auto prependIncludes = parser.values(prependIncludeOption);
        for (const QString &include : prependIncludes)
            moc.includeFiles.prepend(QFile::encodeName(include));
        if (parser.isSet(pathPrefixOption))
            moc.includePath = QFile::encodeName(parser.value(pathPrefixOption));
    }

    const auto includePaths = parser.values(includePathOption);
    for (const QString &path : includePaths)
        pp.includes += Preprocessor::IncludePath(QFile::encodeName(path));
    QString compilerFlavor = parser.value(compilerFlavorOption);
    if (compilerFlavor.isEmpty() || compilerFlavor == QLatin1String("unix")) {
        // traditional Unix compilers use both CPATH and CPLUS_INCLUDE_PATH
        // $CPATH feeds to #include <...> and #include "...", whereas
        // CPLUS_INCLUDE_PATH is equivalent to GCC's -isystem, so we parse later
        const auto cpath = qgetenv("CPATH").split(QDir::listSeparator().toLatin1());
        for (const QByteArray &p : cpath)
            pp.includes += Preprocessor::IncludePath(p);
        const auto cplus_include_path = qgetenv("CPLUS_INCLUDE_PATH").split(QDir::listSeparator().toLatin1());
        for (const QByteArray &p : cplus_include_path)
            pp.includes += Preprocessor::IncludePath(p);
    } else if (compilerFlavor == QLatin1String("msvc")) {
        // MSVC uses one environment variable: INCLUDE
        const auto include = qgetenv("INCLUDE").split(QDir::listSeparator().toLatin1());
        for (const QByteArray &p : include)
            pp.includes += Preprocessor::IncludePath(p);
    } else {
        error(qPrintable(QLatin1String("Unknown compiler flavor '") + compilerFlavor +
                         QLatin1String("'; valid values are: msvc, unix.")));
        parser.showHelp(1);
    }

    const auto macFrameworks = parser.values(macFrameworkOption);
    for (const QString &path : macFrameworks) {
        // minimalistic framework support for the mac
        Preprocessor::IncludePath p(QFile::encodeName(path));
        p.isFrameworkPath = true;
        pp.includes += p;
    }
    const auto defines = parser.values(defineOption);
    for (const QString &arg : defines) {
        QByteArray name = arg.toLocal8Bit();
        QByteArray value("1");
        int eq = name.indexOf('=');
        if (eq >= 0) {
            value = name.mid(eq + 1);
            name = name.left(eq);
        }
        if (name.isEmpty()) {
            error("Missing macro name");
            parser.showHelp(1);
        }
        Macro macro;
        macro.symbols = Preprocessor::tokenize(value, 1, Preprocessor::TokenizeDefine);
        macro.symbols.removeLast(); // remove the EOF symbol
        pp.macros.insert(name, macro);
    }
    const auto undefines = parser.values(undefineOption);
    for (const QString &arg : undefines) {
        QByteArray macro = arg.toLocal8Bit();
        if (macro.isEmpty()) {
            error("Missing macro name");
            parser.showHelp(1);
        }
        pp.macros.remove(macro);
    }
    const QStringList noNotesCompatValues = parser.values(noNotesWarningsCompatOption);
    if (parser.isSet(noNotesOption) || noNotesCompatValues.contains(QLatin1String("n")))
        moc.displayNotes = false;
    if (parser.isSet(noWarningsOption) || noNotesCompatValues.contains(QLatin1String("w")))
        moc.displayWarnings = moc.displayNotes = false;

    if (autoInclude) {
        int spos = filename.lastIndexOf(QDir::separator());
        int ppos = filename.lastIndexOf(QLatin1Char('.'));
        // spos >= -1 && ppos > spos => ppos >= 0
        moc.noInclude = (ppos > spos && filename.at(ppos + 1).toLower() != QLatin1Char('h'));
    }
    if (defaultInclude) {
        if (moc.includePath.isEmpty()) {
            if (filename.size()) {
                if (output.size())
                    moc.includeFiles.append(combinePath(filename, output));
                else
                    moc.includeFiles.append(QFile::encodeName(filename));
            }
        } else {
            moc.includeFiles.append(combinePath(filename, filename));
        }
    }

    if (filename.isEmpty()) {
        filename = QStringLiteral("standard input");
        in.open(stdin, QIODevice::ReadOnly);
    } else {
        in.setFileName(filename);
        if (!in.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "moc: %s: No such file\n", qPrintable(filename));
            return 1;
        }
        moc.filename = filename.toLocal8Bit();
    }

    const auto metadata = parser.values(metadataOption);
    for (const QString &md : metadata) {
        int split = md.indexOf(QLatin1Char('='));
        QString key = md.left(split);
        QString value = md.mid(split + 1);

        if (split == -1 || key.isEmpty() || value.isEmpty()) {
            error("missing key or value for option '-M'");
        } else if (key.indexOf(QLatin1Char('.')) != -1) {
            // Don't allow keys with '.' for now, since we might need this
            // format later for more advanced meta data API
            error("A key cannot contain the letter '.' for option '-M'");
        } else {
            QJsonArray array = moc.metaArgs.value(key);
            array.append(value);
            moc.metaArgs.insert(key, array);
        }
    }

    moc.currentFilenames.push(filename.toLocal8Bit());
    moc.includes = pp.includes;

    // 1. preprocess
    const auto includeFiles = parser.values(includeOption);
    QStringList validIncludesFiles;
    for (const QString &includeName : includeFiles) {
        QByteArray rawName = pp.resolveInclude(QFile::encodeName(includeName), moc.filename);
        if (rawName.isEmpty()) {
            fprintf(stderr, "Warning: Failed to resolve include \"%s\" for moc file %s\n",
                    includeName.toLocal8Bit().constData(),
                    moc.filename.isEmpty() ? "<standard input>" : moc.filename.constData());
        } else {
            QFile f(QFile::decodeName(rawName));
            if (f.open(QIODevice::ReadOnly)) {
                moc.symbols += Symbol(0, MOC_INCLUDE_BEGIN, rawName);
                moc.symbols += pp.preprocessed(rawName, &f);
                moc.symbols += Symbol(0, MOC_INCLUDE_END, rawName);
                validIncludesFiles.append(includeName);
            } else {
                fprintf(stderr, "Warning: Cannot open %s included by moc file %s: %s\n",
                        rawName.constData(),
                        moc.filename.isEmpty() ? "<standard input>" : moc.filename.constData(),
                        f.errorString().toLocal8Bit().constData());
            }
        }
    }
    moc.symbols += pp.preprocessed(moc.filename, &in);

    if (!pp.preprocessOnly) {
        // 2. parse
        moc.parse();
    }

    // 3. and output meta object code

    QScopedPointer<FILE, ScopedPointerFileCloser> jsonOutput;

    bool outputToFile = true;
    if (output.size()) { // output file specified
#if defined(_MSC_VER)
        if (_wfopen_s(&out, reinterpret_cast<const wchar_t *>(output.utf16()), L"w") != 0)
#else
        out = fopen(QFile::encodeName(output).constData(), "w"); // create output file
        if (!out)
#endif
        {
            fprintf(stderr, "moc: Cannot create %s\n", QFile::encodeName(output).constData());
            return 1;
        }

        if (parser.isSet(jsonOption)) {
            const QString jsonOutputFileName = output + QLatin1String(".json");
            FILE *f;
#if defined(_MSC_VER)
            if (_wfopen_s(&f, reinterpret_cast<const wchar_t *>(jsonOutputFileName.utf16()), L"w") != 0)
#else
            f = fopen(QFile::encodeName(jsonOutputFileName).constData(), "w");
            if (!f)
#endif
                fprintf(stderr, "moc: Cannot create JSON output file %s. %s\n",
                        QFile::encodeName(jsonOutputFileName).constData(),
                        strerror(errno));
            jsonOutput.reset(f);
        }
    } else { // use stdout
        out = stdout;
        outputToFile = false;
    }

    if (pp.preprocessOnly) {
        fprintf(out, "%s\n", composePreprocessorOutput(moc.symbols).constData());
    } else {
        if (moc.classList.isEmpty())
            moc.note("No relevant classes found. No output generated.");
        else
            moc.generate(out, jsonOutput.data());
    }

    if (output.size())
        fclose(out);

    if (parser.isSet(depFileOption)) {
        // 4. write a Make-style dependency file (can also be consumed by Ninja).
        QString depOutputFileName;
        QString depRuleName = output;

        if (parser.isSet(depFileRuleNameOption))
            depRuleName = parser.value(depFileRuleNameOption);

        if (parser.isSet(depFilePathOption)) {
            depOutputFileName = parser.value(depFilePathOption);
        } else if (outputToFile) {
            depOutputFileName = output + QLatin1String(".d");
        } else {
            fprintf(stderr, "moc: Writing to stdout, but no depfile path specified.\n");
        }

        QScopedPointer<FILE, ScopedPointerFileCloser> depFileHandle;
        FILE *depFileHandleRaw;
#if defined(_MSC_VER)
        if (_wfopen_s(&depFileHandleRaw,
                      reinterpret_cast<const wchar_t *>(depOutputFileName.utf16()), L"w") != 0)
#else
        depFileHandleRaw = fopen(QFile::encodeName(depOutputFileName).constData(), "w");
        if (!depFileHandleRaw)
#endif
            fprintf(stderr, "moc: Cannot create dep output file '%s'. %s\n",
                    QFile::encodeName(depOutputFileName).constData(),
                    strerror(errno));
        depFileHandle.reset(depFileHandleRaw);

        if (!depFileHandle.isNull()) {
            // First line is the path to the generated file.
            fprintf(depFileHandle.data(), "%s: ",
                    escapeAndEncodeDependencyPath(depRuleName).constData());

            QByteArrayList dependencies;

            // If there's an input file, it's the first dependency.
            if (!filename.isEmpty()) {
                dependencies.append(escapeAndEncodeDependencyPath(filename).constData());
            }

            // Additional passed-in includes are dependencies (like moc_predefs.h).
            for (const QString &includeName : validIncludesFiles) {
                dependencies.append(escapeAndEncodeDependencyPath(includeName).constData());
            }

            // Plugin metadata json files discovered via Q_PLUGIN_METADATA macros are also
            // dependencies.
            for (const QString &pluginMetadataFile : moc.parsedPluginMetadataFiles) {
                dependencies.append(escapeAndEncodeDependencyPath(pluginMetadataFile).constData());
            }

            // All pre-processed includes are dependnecies.
            // Sort the entries for easier human consumption.
            auto includeList = pp.preprocessedIncludes.values();
            std::sort(includeList.begin(), includeList.end());

            for (QByteArray &includeName : includeList) {
                dependencies.append(escapeDependencyPath(includeName));
            }

            // Join dependencies, output them, and output a final new line.
            const auto dependenciesJoined = dependencies.join(QByteArrayLiteral(" \\\n  "));
            fprintf(depFileHandle.data(), "%s\n", dependenciesJoined.constData());
        }
    }

    return 0;
}

QT_END_NAMESPACE

int main(int _argc, char **_argv)
{
    return QT_PREPEND_NAMESPACE(runMoc)(_argc, _argv);
}
