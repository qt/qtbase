// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*
 * The tool generates deployment artifacts for the Qt builds such as:
 *     - CaMeL case header files named by public C++ symbols located in public module header files
 *     - Header file that contains the module version information, and named as <module>Vesion
 *     - LD version script if applicable
 *     - Aliases or copies of the header files sorted by the generic Qt-types: public/private/qpa
 * and stored in the corresponding directories.
 * Also the tool executes conformity checks on each header file if applicable, to make sure they
 * follow rules that are relevant for their header type.
 * The tool can be run in two modes: with either '-all' or '-headers' options specified. Depending
 * on the selected mode, the tool either scans the filesystem to find header files or use the
 * pre-defined list of header files.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <cstring>
#include <sstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <regex>
#include <map>
#include <set>
#include <stdexcept>
#include <array>

enum ErrorCodes {
    NoError = 0,
    InvalidArguments,
    SyncFailed,
};

// Enum contains the list of checks that can be executed on header files.
enum HeaderChecks {
    NoChecks = 0,
    NamespaceChecks = 1, /* Checks if header file is wrapped with QT_<BEGIN|END>_NAMESPACE macros */
    PrivateHeaderChecks = 2, /* Checks if the public header includes a private header */
    IncludeChecks = 4, /* Checks if the real header file but not an alias is included */
    WeMeantItChecks = 8, /* Checks if private header files contains 'We meant it' disclaimer */
    PragmaOnceChecks = 16,
    /* Checks that lead to the fatal error of the sync process: */
    CriticalChecks = PrivateHeaderChecks | PragmaOnceChecks,
    AllChecks = NamespaceChecks | CriticalChecks | IncludeChecks | WeMeantItChecks,
};

constexpr int LinkerScriptCommentAlignment = 55;

static const std::regex GlobalHeaderRegex("^q(.*)global\\.h$");

constexpr std::string_view ErrorMessagePreamble = "ERROR: ";
constexpr std::string_view WarningMessagePreamble = "WARNING: ";

// This comparator is used to sort include records in master header.
// It's used to put q.*global.h file to the top of the list and sort all other files alphabetically.
bool MasterHeaderIncludeComparator(const std::string &a, const std::string &b)
{
    std::smatch amatch;
    std::smatch bmatch;

    if (std::regex_match(a, amatch, GlobalHeaderRegex)) {
        if (std::regex_match(b, bmatch, GlobalHeaderRegex)) {
            return amatch[1].str().empty()
                    || (!bmatch[1].str().empty() && amatch[1].str() < bmatch[1].str());
        }
        return true;
    } else if (std::regex_match(b, bmatch, GlobalHeaderRegex)) {
        return false;
    }

    return a < b;
};

namespace utils {
std::string asciiToLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (c >= 'A' && c <= 'Z') ? c | 0x20 : c; });
    return s;
}

std::string asciiToUpper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return (c >= 'a' && c <= 'z') ? c & 0xdf : c; });
    return s;
}

bool parseVersion(const std::string &version, int &major, int &minor)
{
    const size_t separatorPos = version.find('.');
    if (separatorPos == std::string::npos || separatorPos == (version.size() - 1)
        || separatorPos == 0)
        return false;

    try {
        size_t pos = 0;
        major = std::stoi(version.substr(0, separatorPos), &pos);
        if (pos != separatorPos)
            return false;

        const size_t nextPart = separatorPos + 1;
        pos = 0;
        minor = std::stoi(version.substr(nextPart), &pos);
        if (pos != (version.size() - nextPart))
            return false;
    } catch (const std::invalid_argument &) {
        return false;
    } catch (const std::out_of_range &) {
        return false;
    }

    return true;
}

class DummyOutputStream : public std::ostream
{
    struct : public std::streambuf
    {
        int overflow(int c) override { return c; }
    } buff;

public:
    DummyOutputStream() : std::ostream(&buff) { }
} DummyOutput;

void printInternalError()
{
    std::cerr << "Internal error. Please create bugreport at https://bugreports.qt.io "
                 "using 'Build tools: Other component.'"
              << std::endl;
}

void printFilesystemError(const std::filesystem::filesystem_error &fserr, std::string_view errorMsg)
{
    std::cerr << errorMsg << ": " << fserr.path1() << ".\n"
              << fserr.what() << "(" << fserr.code().value() << ")" << std::endl;
}

std::filesystem::path normilizedPath(const std::string &path)
{
    try {
        auto result = std::filesystem::path(std::filesystem::weakly_canonical(path).generic_string());
        return result;
    } catch (const std::filesystem::filesystem_error &fserr) {
        printFilesystemError(fserr, "Unable to normalize path");
        throw;
    }
}

bool createDirectories(const std::string &path, std::string_view errorMsg, bool *exists = nullptr)
{
    bool result = true;
    try {
        if (!std::filesystem::exists(path)) {
            if (exists)
                *exists = false;
            std::filesystem::create_directories(path);
        } else {
            if (exists)
                *exists = true;
        }
    } catch (const std::filesystem::filesystem_error &fserr) {
        result = false;
        std::cerr << errorMsg << ": " << path << ".\n"
                  << fserr.code().message() << "(" << fserr.code().value() << "):" << fserr.what()
                  << std::endl;
    }
    return result;
}

} // namespace utils

using FileStamp = std::filesystem::file_time_type;

class CommandLineOptions
{
    template<typename T>
    struct CommandLineOption
    {
        CommandLineOption(T *_value, bool _isOptional = false)
            : value(_value), isOptional(_isOptional)
        {
        }

        T *value;
        bool isOptional;
    };

public:
    CommandLineOptions(int argc, char *argv[]) : m_isValid(parseArguments(argc, argv)) { }

    bool isValid() const { return m_isValid; }

    const std::string &moduleName() const { return m_moduleName; }

    const std::string &sourceDir() const { return m_sourceDir; }

    const std::string &binaryDir() const { return m_binaryDir; }

    const std::string &includeDir() const { return m_includeDir; }

    const std::string &installIncludeDir() const { return m_installIncludeDir; }

    const std::string &privateIncludeDir() const { return m_privateIncludeDir; }

    const std::string &qpaIncludeDir() const { return m_qpaIncludeDir; }

    const std::string &rhiIncludeDir() const { return m_rhiIncludeDir; }

    const std::string &ssgIncludeDir() const { return m_ssgIncludeDir; }

    const std::string &stagingDir() const { return m_stagingDir; }

    const std::string &versionScriptFile() const { return m_versionScriptFile; }

    const std::set<std::string> &knownModules() const { return m_knownModules; }

    const std::regex &qpaHeadersRegex() const { return m_qpaHeadersRegex; }

    const std::regex &rhiHeadersRegex() const { return m_rhiHeadersRegex; }

    const std::regex &ssgHeadersRegex() const { return m_ssgHeadersRegex; }

    const std::regex &privateHeadersRegex() const { return m_privateHeadersRegex; }

    const std::regex &publicNamespaceRegex() const { return m_publicNamespaceRegex; }

    const std::set<std::string> &headers() const { return m_headers; }

    const std::set<std::string> &generatedHeaders() const { return m_generatedHeaders; }

    bool scanAllMode() const { return m_scanAllMode; }

    bool isInternal() const { return m_isInternal; }

    bool isNonQtModule() const { return m_isNonQtModule; }

    bool printHelpOnly() const { return m_printHelpOnly; }

    bool debug() const { return m_debug; }

    bool copy() const { return m_copy; }

    bool minimal() const { return m_minimal; }

    bool showOnly() const { return m_showOnly; }

    bool warningsAreErrors() const { return m_warningsAreErrors; }

    void printHelp() const
    {
        std::cout << "Usage: syncqt -sourceDir <dir> -binaryDir <dir> -module <module name>"
                     " -includeDir <dir> -privateIncludeDir <dir> -qpaIncludeDir <dir> -rhiIncludeDir <dir> -ssgIncludeDir <dir>"
                     " -stagingDir <dir> <-headers <header list>|-all> [-debug]"
                     " [-versionScript <path>] [-qpaHeadersFilter <regex>] [-rhiHeadersFilter <regex>]"
                     " [-knownModules <module1> <module2>... <moduleN>]"
                     " [-nonQt] [-internal] [-copy]\n"
                     ""
                     "Mandatory arguments:\n"
                     "  -module                         Module name.\n"
                     "  -headers                        List of header files.\n"
                     "  -all                            In 'all' mode syncqt scans source\n"
                     "                                  directory for public qt headers and\n"
                     "                                  artifacts not considering CMake source\n"
                     "                                  tree. The main use cases are the \n"
                     "                                  generating of documentation and creating\n"
                     "                                  API review changes.\n"
                     "  -sourceDir                      Module source directory.\n"
                     "  -binaryDir                      Module build directory.\n"
                     "  -includeDir                     Module include directory where the\n"
                     "                                  generated header files will be located.\n"
                     "  -privateIncludeDir              Module include directory for the\n"
                     "                                  generated private header files.\n"
                     "  -qpaIncludeDir                  Module include directory for the \n"
                     "                                  generated QPA header files.\n"
                     "  -rhiIncludeDir                  Module include directory for the \n"
                     "                                  generated RHI header files.\n"
                     "  -ssgIncludeDir                  Module include directory for the \n"
                     "                                  generated SSG header files.\n"
                     "  -stagingDir                     Temporary staging directory to collect\n"
                     "                                  artifacts that need to be installed.\n"
                     "  -knownModules                   list of known modules. syncqt uses the\n"
                     "                                  list to check the #include macros\n"
                     "                                  consistency.\n"
                     "Optional arguments:\n"
                     "  -internal                       Indicates that the module is internal.\n"
                     "  -nonQt                          Indicates that the module is not a Qt\n"
                     "                                  module.\n"
                     "  -privateHeadersFilter           Regex that filters private header files\n"
                     "                                  from the list of 'headers'.\n"
                     "  -qpaHeadersFilter               Regex that filters qpa header files from.\n"
                     "                                  the list of 'headers'.\n"
                     "  -rhiHeadersFilter               Regex that filters rhi header files from.\n"
                     "                                  the list of 'headers'.\n"
                     "  -ssgHeadersFilter               Regex that filters ssg files from.\n"
                     "                                  the list of 'headers'.\n"
                     "  -publicNamespaceFilter          Symbols that are in the specified\n"
                     "                                  namespace.\n"
                     "                                  are treated as public symbols.\n"
                     "  -versionScript                  Generate linker version script by\n"
                     "                                  provided path.\n"
                     "  -debug                          Enable debug output.\n"
                     "  -copy                           Copy header files instead of creating\n"
                     "                                  aliases.\n"
                     "  -minimal                        Do not create CaMeL case headers for the\n"
                     "                                  public C++ symbols.\n"
                     "  -showonly                       Show actions, but not perform them.\n"
                     "  -warningsAreErrors              Treat all warnings as errors.\n"
                     "  -help                           Print this help.\n";
    }

private:
    template<typename T>
    [[nodiscard]] bool checkRequiredArguments(const std::unordered_map<std::string, T> &arguments)
    {
        bool ret = true;
        for (const auto &argument : arguments) {
            if (!argument.second.isOptional
                && (!argument.second.value || argument.second.value->size()) == 0) {
                std::cerr << "Missing argument: " << argument.first << std::endl;
                ret = false;
            }
        }
        return ret;
    }

    [[nodiscard]] bool parseArguments(int argc, char *argv[])
    {
        std::string qpaHeadersFilter;
        std::string rhiHeadersFilter;
        std::string ssgHeadersFilter;
        std::string privateHeadersFilter;
        std::string publicNamespaceFilter;
        const std::unordered_map<std::string, CommandLineOption<std::string>> stringArgumentMap = {
            { "-module", { &m_moduleName } },
            { "-sourceDir", { &m_sourceDir } },
            { "-binaryDir", { &m_binaryDir } },
            { "-installIncludeDir", { &m_installIncludeDir, true } },
            { "-privateHeadersFilter", { &privateHeadersFilter, true } },
            { "-qpaHeadersFilter", { &qpaHeadersFilter, true } },
            { "-rhiHeadersFilter", { &rhiHeadersFilter, true } },
            { "-ssgHeadersFilter", { &ssgHeadersFilter, true } },
            { "-includeDir", { &m_includeDir } },
            { "-privateIncludeDir", { &m_privateIncludeDir } },
            { "-qpaIncludeDir", { &m_qpaIncludeDir } },
            { "-rhiIncludeDir", { &m_rhiIncludeDir } },
            { "-ssgIncludeDir", { &m_ssgIncludeDir } },
            { "-stagingDir", { &m_stagingDir, true } },
            { "-versionScript", { &m_versionScriptFile, true } },
            { "-publicNamespaceFilter", { &publicNamespaceFilter, true } },
        };

        const std::unordered_map<std::string, CommandLineOption<std::set<std::string>>>
                listArgumentMap = {
                    { "-headers", { &m_headers, true } },
                    { "-generatedHeaders", { &m_generatedHeaders, true } },
                    { "-knownModules", { &m_knownModules, true } },
                };

        const std::unordered_map<std::string, CommandLineOption<bool>> boolArgumentMap = {
            { "-nonQt", { &m_isNonQtModule, true } }, { "-debug", { &m_debug, true } },
            { "-help", { &m_printHelpOnly, true } },
            { "-internal", { &m_isInternal, true } }, { "-all", { &m_scanAllMode, true } },
            { "-copy", { &m_copy, true } },           { "-minimal", { &m_minimal, true } },
            { "-showonly", { &m_showOnly, true } },   { "-showOnly", { &m_showOnly, true } },
            { "-warningsAreErrors", { &m_warningsAreErrors, true } }
        };

        std::string *currentValue = nullptr;
        std::set<std::string> *currentListValue = nullptr;

        auto parseArgument = [&](const std::string &arg) -> bool {
            if (arg[0] == '-') {
                currentValue = nullptr;
                currentListValue = nullptr;
                {
                    auto it = stringArgumentMap.find(arg);
                    if (it != stringArgumentMap.end()) {
                        if (it->second.value == nullptr) {
                            utils::printInternalError();
                            return false;
                        }
                        currentValue = it->second.value;
                        return true;
                    }
                }

                {
                    auto it = boolArgumentMap.find(arg);
                    if (it != boolArgumentMap.end()) {
                        if (it->second.value == nullptr) {
                            utils::printInternalError();
                            return false;
                        }
                        *(it->second.value) = true;
                        return true;
                    }
                }

                {
                    auto it = listArgumentMap.find(arg);
                    if (it != listArgumentMap.end()) {
                        if (it->second.value == nullptr) {
                            utils::printInternalError();
                            return false;
                        }
                        currentListValue = it->second.value;
                        currentListValue->insert(""); // Indicate that argument is provided
                        return true;
                    }
                }

                std::cerr << "Unknown argument: " << arg << std::endl;
                return false;
            }

            if (currentValue != nullptr) {
                *currentValue = arg;
                currentValue = nullptr;
            } else if (currentListValue != nullptr) {
                currentListValue->insert(arg);
            } else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                return false;
            }
            return true;
        };

        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg.empty())
                continue;

            if (arg[0] == '@') {
                std::ifstream ifs(arg.substr(1), std::ifstream::in);
                if (!ifs.is_open()) {
                    std::cerr << "Unable to open rsp file: " << arg[0] << std::endl;
                    return false;
                }
                std::string argFromFile;
                while (std::getline(ifs, argFromFile)) {
                    if (argFromFile.empty())
                        continue;
                    if (!parseArgument(argFromFile))
                        return false;
                }
                ifs.close();
                continue;
            }

            if (!parseArgument(arg))
                return false;
        }

        if (m_printHelpOnly)
            return true;

        if (!qpaHeadersFilter.empty())
            m_qpaHeadersRegex = std::regex(qpaHeadersFilter);

        if (!rhiHeadersFilter.empty())
            m_rhiHeadersRegex = std::regex(rhiHeadersFilter);

        if (!ssgHeadersFilter.empty())
            m_ssgHeadersRegex = std::regex(ssgHeadersFilter);

        if (!privateHeadersFilter.empty())
            m_privateHeadersRegex = std::regex(privateHeadersFilter);

        if (!publicNamespaceFilter.empty())
            m_publicNamespaceRegex = std::regex(publicNamespaceFilter);

        if (m_headers.empty() && !m_scanAllMode) {
            std::cerr << "You need to specify either -headers or -all option." << std::endl;
            return false;
        }

        if (!m_headers.empty() && m_scanAllMode) {
            std::cerr << "Both -headers and -all are specified. Need to choose only one"
                         "operational mode." << std::endl;
            return false;
        }

        for (const auto &argument : listArgumentMap)
            argument.second.value->erase("");

        bool ret = true;
        ret &= checkRequiredArguments(stringArgumentMap);
        ret &= checkRequiredArguments(listArgumentMap);

        normilizePaths();

        return ret;
    }

    // Convert all paths from command line to a generic one.
    void normilizePaths()
    {
        const std::array paths = {
            &m_sourceDir,         &m_binaryDir,         &m_includeDir,
            &m_installIncludeDir, &m_privateIncludeDir, &m_qpaIncludeDir,
            &m_rhiIncludeDir,     &m_stagingDir,        &m_versionScriptFile,
        };
        for (auto path : paths) {
            if (!path->empty())
                *path = utils::normilizedPath(*path).generic_string();
        }
    }

    std::string m_moduleName;
    std::string m_sourceDir;
    std::string m_binaryDir;
    std::string m_includeDir;
    std::string m_installIncludeDir;
    std::string m_privateIncludeDir;
    std::string m_qpaIncludeDir;
    std::string m_rhiIncludeDir;
    std::string m_ssgIncludeDir;
    std::string m_stagingDir;
    std::string m_versionScriptFile;
    std::set<std::string> m_knownModules;
    std::set<std::string> m_headers;
    std::set<std::string> m_generatedHeaders;
    bool m_scanAllMode = false;
    bool m_copy = false;
    bool m_isNonQtModule = false;
    bool m_isInternal = false;
    bool m_printHelpOnly = false;
    bool m_debug = false;
    bool m_minimal = false;
    bool m_showOnly = false;
    bool m_warningsAreErrors = false;
    std::regex m_qpaHeadersRegex;
    std::regex m_rhiHeadersRegex;
    std::regex m_ssgHeadersRegex;
    std::regex m_privateHeadersRegex;
    std::regex m_publicNamespaceRegex;

    bool m_isValid;
};

class SyncScanner
{
    class SymbolDescriptor
    {
    public:
        // Where the symbol comes from
        enum SourceType {
            Pragma = 0, // pragma qt_class is mentioned a header file
            Declaration, // The symbol declaration inside a header file
            MaxSourceType
        };

        void update(const std::string &file, SourceType type)
        {
            if (type < m_type) {
                m_file = file;
                m_type = type;
            }
        }

        // The file that contains a symbol.
        const std::string &file() const { return m_file; }

    private:
        SourceType m_type = MaxSourceType;
        std::string m_file;
    };
    using SymbolContainer = std::unordered_map<std::string, SymbolDescriptor>;

    struct ParsingResult
    {
        std::vector<std::string> versionScriptContent;
        std::string requireConfig;
        bool masterInclude = true;
    };

    CommandLineOptions *m_commandLineArgs = nullptr;

    std::map<std::string /* header file name */, std::string /* header feature guard name */,
             decltype(MasterHeaderIncludeComparator) *>
            m_masterHeaderContents;

    std::unordered_map<std::string /* the deprecated header name*/,
                       std::string /* the replacement */>
            m_deprecatedHeaders;
    std::vector<std::string> m_versionScriptContents;
    std::set<std::string> m_producedHeaders;
    std::vector<std::string> m_headerCheckExceptions;
    SymbolContainer m_symbols;
    std::ostream &scannerDebug() const
    {
        if (m_commandLineArgs->debug())
            return std::cout;
        return utils::DummyOutput;
    }

    enum { Active, Stopped, IgnoreNext, Ignore } m_versionScriptGeneratorState = Active;

    std::filesystem::path m_outputRootName;
    std::filesystem::path m_currentFile;
    std::string m_currentFilename;
    std::string m_currentFileString;
    size_t m_currentFileLineNumber = 0;
    bool m_currentFileInSourceDir = false;

    enum FileType { PublicHeader = 0, PrivateHeader = 1, QpaHeader = 2, ExportHeader = 4, RhiHeader = 8, SsgHeader = 16 };
    unsigned int m_currentFileType = PublicHeader;

    int m_criticalChecks = CriticalChecks;
    std::string_view m_warningMessagePreamble;

public:
    SyncScanner(CommandLineOptions *commandLineArgs)
        : m_commandLineArgs(commandLineArgs),
          m_masterHeaderContents(MasterHeaderIncludeComparator),
          m_outputRootName(
                  std::filesystem::weakly_canonical(m_commandLineArgs->includeDir()).root_name()),
          m_warningMessagePreamble(WarningMessagePreamble)
    {
    }

    // The function converts the relative path to a header files to the absolute. It also makes the
    // path canonical(removes '..' and '.' parts of the path). The source directory passed in
    // '-sourceDir' command line argument is used as base path for relative paths to create the
    // absolute path.
    [[nodiscard]] std::filesystem::path makeHeaderAbsolute(const std::string &filename) const;

    ErrorCodes sync()
    {
        if (m_commandLineArgs->warningsAreErrors()) {
            m_criticalChecks = AllChecks;
            m_warningMessagePreamble = ErrorMessagePreamble;
        }

        m_versionScriptGeneratorState =
                m_commandLineArgs->versionScriptFile().empty() ? Stopped : Active;
        auto error = NoError;

        // In the scan all mode we ingore the list of header files that is specified in the
        // '-headers' argument, and collect header files from the source directory tree.
        if (m_commandLineArgs->scanAllMode()) {
            for (auto const &entry :
                 std::filesystem::recursive_directory_iterator(m_commandLineArgs->sourceDir())) {

                const bool isRegularFile = entry.is_regular_file();
                const bool isHeaderFlag = isHeader(entry);
                const bool isDocFileHeuristicFlag =
                    isDocFileHeuristic(entry.path().generic_string());
                const bool shouldProcessHeader =
                    isRegularFile && isHeaderFlag && !isDocFileHeuristicFlag;
                const std::string filePath = entry.path().generic_string();

                if (shouldProcessHeader) {
                    scannerDebug() << "Processing header: " << filePath << std::endl;
                    if (!processHeader(makeHeaderAbsolute(filePath)))
                        error = SyncFailed;
                } else {
                    scannerDebug()
                        << "Skipping processing header: " << filePath
                        << " isRegularFile: " << isRegularFile
                        << " isHeaderFlag: " << isHeaderFlag
                        << " isDocFileHeuristicFlag: " << isDocFileHeuristicFlag
                        << std::endl;
                }
            }
        } else {
            // Since the list of header file is quite big syncqt supports response files to avoid
            // the issues with long command lines.
            std::set<std::string> rspHeaders;
            const auto &headers = m_commandLineArgs->headers();
            for (auto it = headers.begin(); it != headers.end(); ++it) {
                const auto &header = *it;
                scannerDebug() << "Processing header: " << header << std::endl;
                if (!processHeader(makeHeaderAbsolute(header))) {
                    error = SyncFailed;
                }
            }
            for (const auto &header : rspHeaders) {
                scannerDebug() << "Processing header: " << header << std::endl;
                if (!processHeader(makeHeaderAbsolute(header)))
                    error = SyncFailed;
            }
        }

        // No further processing in minimal mode.
        if (m_commandLineArgs->minimal())
            return error;

        // Generate aliases for all unique symbols collected during the header files parsing.
        for (auto it = m_symbols.begin(); it != m_symbols.end(); ++it) {
            const std::string &filename = it->second.file();
            if (!filename.empty()) {
                if (generateQtCamelCaseFileIfContentChanged(
                            m_commandLineArgs->includeDir() + '/' + it->first, filename)) {
                    m_producedHeaders.insert(it->first);
                } else {
                    error = SyncFailed;
                }
            }
        }

        // Generate the header file containing version information.
        if (!m_commandLineArgs->isNonQtModule()) {
            std::string moduleNameLower = utils::asciiToLower(m_commandLineArgs->moduleName());
            std::string versionHeaderFilename(moduleNameLower + "version.h");
            std::string versionHeaderCamel(m_commandLineArgs->moduleName() + "Version");
            std::string versionFile = m_commandLineArgs->includeDir() + '/' + versionHeaderFilename;

            std::error_code ec;
            FileStamp originalStamp = std::filesystem::last_write_time(versionFile, ec);
            if (ec)
                originalStamp = FileStamp::clock::now();

            if (generateVersionHeader(versionFile)) {
                if (!generateAliasedHeaderFileIfTimestampChanged(
                            m_commandLineArgs->includeDir() + '/' + versionHeaderCamel,
                            versionHeaderFilename, originalStamp)) {
                    error = SyncFailed;
                }
                m_masterHeaderContents[versionHeaderFilename] = {};
                m_producedHeaders.insert(versionHeaderFilename);
                m_producedHeaders.insert(versionHeaderCamel);
            } else {
                error = SyncFailed;
            }
        }

        if (!m_commandLineArgs->scanAllMode()) {
            if (!m_commandLineArgs->isNonQtModule()) {
                if (!generateDeprecatedHeaders())
                    error = SyncFailed;

                if (!generateHeaderCheckExceptions())
                    error = SyncFailed;
            }

            if (!m_commandLineArgs->versionScriptFile().empty()) {
                if (!generateLinkerVersionScript())
                    error = SyncFailed;
            }
        }

        if (!m_commandLineArgs->isNonQtModule()) {
            if (!generateMasterHeader())
                error = SyncFailed;
        }

        if (!m_commandLineArgs->scanAllMode() && !m_commandLineArgs->stagingDir().empty()) {
            // Copy the generated files to a spearate staging directory to make the installation
            // process eaiser.
            if (!copyGeneratedHeadersToStagingDirectory(m_commandLineArgs->stagingDir()))
                error = SyncFailed;
        }
        return error;
    }

    // The function copies files, that were generated while the sync procedure to a staging
    // directory. This is necessary to simplify the installation of the generated files.
    [[nodiscard]] bool copyGeneratedHeadersToStagingDirectory(const std::string &outputDirectory,
                                                              bool skipCleanup = false)
    {
        bool result = true;
        bool outDirExists = false;
        if (!utils::createDirectories(outputDirectory, "Unable to create staging directory",
                                      &outDirExists))
            return false;

        if (outDirExists && !skipCleanup) {
            try {
                for (const auto &entry :
                     std::filesystem::recursive_directory_iterator(outputDirectory)) {
                    if (m_producedHeaders.find(entry.path().filename().generic_string())
                        == m_producedHeaders.end()) {
                        // Check if header file came from another module as result of the
                        // cross-module deprecation before removing it.
                        std::string firstLine;
                        {
                            std::ifstream input(entry.path(), std::ifstream::in);
                            if (input.is_open()) {
                                std::getline(input, firstLine);
                                input.close();
                            }
                        }
                        if (firstLine.find("#ifndef DEPRECATED_HEADER_"
                                           + m_commandLineArgs->moduleName())
                                    == 0
                            || firstLine.find("#ifndef DEPRECATED_HEADER_") != 0)
                            std::filesystem::remove(entry.path());
                    }
                }
            } catch (const std::filesystem::filesystem_error &fserr) {
                utils::printFilesystemError(fserr, "Unable to clean the staging directory");
                return false;
            }
        }

        for (const auto &header : m_producedHeaders) {
            std::filesystem::path src(m_commandLineArgs->includeDir() + '/' + header);
            std::filesystem::path dst(outputDirectory + '/' + header);
            if (!m_commandLineArgs->showOnly())
                result &= updateOrCopy(src, dst);
        }
        return result;
    }

    void resetCurrentFileInfoData(const std::filesystem::path &headerFile)
    {
        // This regex filters the generated '*exports.h' and '*exports_p.h' header files.
        static const std::regex ExportsHeaderRegex("^q(.*)exports(_p)?\\.h$");

        m_currentFile = headerFile;
        m_currentFileLineNumber = 0;
        m_currentFilename = m_currentFile.filename().generic_string();
        m_currentFileType = PublicHeader;
        m_currentFileString = m_currentFile.generic_string();
        m_currentFileInSourceDir = m_currentFileString.find(m_commandLineArgs->sourceDir()) == 0;

        if (isHeaderPrivate(m_currentFilename))
            m_currentFileType = PrivateHeader;

        if (isHeaderQpa(m_currentFilename))
            m_currentFileType = QpaHeader | PrivateHeader;

        if (isHeaderRhi(m_currentFilename))
            m_currentFileType = RhiHeader | PrivateHeader;

        if (isHeaderSsg(m_currentFilename))
            m_currentFileType = SsgHeader | PrivateHeader;

        if (std::regex_match(m_currentFilename, ExportsHeaderRegex))
            m_currentFileType |= ExportHeader;
    }

    [[nodiscard]] bool processHeader(const std::filesystem::path &headerFile)
    {
        // This regex filters any paths that contain the '3rdparty' directory.
        static const std::regex ThirdPartyFolderRegex("(^|.+/)3rdparty/.+");

        // This regex filters '-config.h' and '-config_p.h' header files.
        static const std::regex ConfigHeaderRegex("^(q|.+-)config(_p)?\\.h");

        resetCurrentFileInfoData(headerFile);
        // We assume that header files ouside of the module source or build directories do not
        // belong to the module. Skip any processing.
        if (!m_currentFileInSourceDir
            && m_currentFileString.find(m_commandLineArgs->binaryDir()) != 0) {
            scannerDebug() << "Header file: " << headerFile
                           << " is outside the sync directories. Skipping." << std::endl;
            m_headerCheckExceptions.push_back(m_currentFileString);
            return true;
        }

        // Check if a directory is passed as argument. That shouldn't happen, print error and exit.
        if (m_currentFilename.empty()) {
            std::cerr << "Header file name of " << m_currentFileString << "is empty" << std::endl;
            return false;
        }

        std::error_code ec;
        FileStamp originalStamp = std::filesystem::last_write_time(headerFile, ec);
        if (ec)
            originalStamp = FileStamp::clock::now();
        ec.clear();

        bool isPrivate = m_currentFileType & PrivateHeader;
        bool isQpa = m_currentFileType & QpaHeader;
        bool isRhi = m_currentFileType & RhiHeader;
        bool isSsg = m_currentFileType & SsgHeader;
        bool isExport = m_currentFileType & ExportHeader;
        scannerDebug()
            << "processHeader:start: " << headerFile
            << " m_currentFilename: " << m_currentFilename
            << " isPrivate: " << isPrivate
            << " isQpa: " << isQpa
            << " isRhi: " << isRhi
            << " isSsg: " << isSsg
            << std::endl;

        // Chose the directory where to generate the header aliases or to copy header file if
        // the '-copy' argument is passed.
        std::string outputDir = m_commandLineArgs->includeDir();
        if (isQpa)
            outputDir = m_commandLineArgs->qpaIncludeDir();
        else if (isRhi)
            outputDir = m_commandLineArgs->rhiIncludeDir();
        else if (isSsg)
            outputDir = m_commandLineArgs->ssgIncludeDir();
        else if (isPrivate)
            outputDir = m_commandLineArgs->privateIncludeDir();

        if (!utils::createDirectories(outputDir, "Unable to create output directory"))
            return false;

        bool headerFileExists = std::filesystem::exists(headerFile);

        std::string aliasedFilepath = headerFile.generic_string();

        std::string aliasPath = outputDir + '/' + m_currentFilename;

        // If the '-copy' argument is passed, we copy the original file to a corresponding output
        // directory otherwise we only create a header file alias that contains relative path to
        // the original header file in the module source or build tree.
        if (m_commandLineArgs->copy() && headerFileExists) {
            if (!updateOrCopy(headerFile, aliasPath))
                return false;
        } else {
            if (!generateAliasedHeaderFileIfTimestampChanged(aliasPath, aliasedFilepath,
                                                             originalStamp))
                return false;
        }

        // No further processing in minimal mode.
        if (m_commandLineArgs->minimal())
            return true;

        // Stop processing if header files doesn't exist. This happens at configure time, since
        // either header files are generated later than syncqt is running or header files only
        // generated at build time. These files will be processed at build time, if CMake files
        // contain the correct dependencies between the missing header files and the module
        // 'sync_headers' targets.
        if (!headerFileExists) {
            scannerDebug() << "Header file: " << headerFile
                           << " doesn't exist, but is added to syncqt scanning. Skipping.";
            return true;
        }

        bool isGenerated = isHeaderGenerated(m_currentFileString);

        // Make sure that we detect the '3rdparty' directory inside the source directory only,
        // since full path to the Qt sources might contain '/3rdparty/' too.
        bool is3rdParty = std::regex_match(
                std::filesystem::relative(headerFile, m_commandLineArgs->sourceDir())
                        .generic_string(),
                ThirdPartyFolderRegex);

        // No processing of generated Qt config header files.
        if (!std::regex_match(m_currentFilename, ConfigHeaderRegex)) {
            unsigned int skipChecks = m_commandLineArgs->scanAllMode() ? AllChecks : NoChecks;

            // Collect checks that should skipped for the header file.
            if (m_commandLineArgs->isNonQtModule() || is3rdParty || isQpa || isRhi || isSsg
                || !m_currentFileInSourceDir || isGenerated) {
                skipChecks = AllChecks;
            } else {
                if (std::regex_match(m_currentFilename, GlobalHeaderRegex) || isExport)
                    skipChecks |= NamespaceChecks;

                if (isHeaderPCH(m_currentFilename))
                    skipChecks |= WeMeantItChecks;

                if (isPrivate) {
                    skipChecks |= NamespaceChecks;
                    skipChecks |= PrivateHeaderChecks;
                    skipChecks |= IncludeChecks;
                } else {
                    skipChecks |= WeMeantItChecks;
                }
            }

            ParsingResult parsingResult;
            parsingResult.masterInclude = m_currentFileInSourceDir && !isExport && !is3rdParty
                    && !isQpa && !isRhi && !isSsg && !isPrivate && !isGenerated;
            if (!parseHeader(headerFile, parsingResult, skipChecks)) {
                scannerDebug() << "parseHeader failed: " << headerFile << std::endl;
                return false;
            }

            // Record the private header file inside the version script content.
            if (isPrivate && !m_commandLineArgs->versionScriptFile().empty()
                && !parsingResult.versionScriptContent.empty()) {
                m_versionScriptContents.insert(m_versionScriptContents.end(),
                                               parsingResult.versionScriptContent.begin(),
                                               parsingResult.versionScriptContent.end());
            }

            // Add the '#if QT_CONFIG(<feature>)' check for header files that supposed to be
            // included into the module master header only if corresponding feature is enabled.
            bool willBeInModuleMasterHeader = false;
            if (!isQpa && !isRhi && !isSsg && !isPrivate) {
                if (m_currentFilename.find('_') == std::string::npos
                    && parsingResult.masterInclude) {
                    m_masterHeaderContents[m_currentFilename] = parsingResult.requireConfig;
                    willBeInModuleMasterHeader = true;
                }
            }

            scannerDebug()
                << "processHeader:end: " << headerFile
                << " is3rdParty: " << is3rdParty
                << " isGenerated: " << isGenerated
                << " m_currentFileInSourceDir: " << m_currentFileInSourceDir
                << " willBeInModuleMasterHeader: " << willBeInModuleMasterHeader
                << std::endl;
        } else if (m_currentFilename == "qconfig.h") {
            // Hardcode generating of QtConfig alias
            updateSymbolDescriptor("QtConfig", "qconfig.h", SyncScanner::SymbolDescriptor::Pragma);
        }

        return true;
    }

    void parseVersionScriptContent(const std::string buffer, ParsingResult &result)
    {
        // This regex looks for the symbols that needs to be placed into linker version script.
        static const std::regex VersionScriptSymbolRegex(
                "^(?:struct|class)(?:\\s+Q_\\w*_EXPORT)?\\s+([\\w:]+)[^;]*(;$)?");

        // This regex looks for the namespaces that needs to be placed into linker version script.
        static const std::regex VersionScriptNamespaceRegex(
                "^namespace\\s+Q_\\w+_EXPORT\\s+([\\w:]+).*");

        // This regex filters the tailing colon from the symbol name.
        static const std::regex TrailingColonRegex("([\\w]+):$");

        switch (m_versionScriptGeneratorState) {
        case Ignore:
            scannerDebug() << "line ignored: " << buffer << std::endl;
            m_versionScriptGeneratorState = Active;
            return;
        case Stopped:
            return;
        case IgnoreNext:
            m_versionScriptGeneratorState = Ignore;
            break;
        case Active:
            break;
        }

        if (buffer.empty())
            return;

        std::smatch match;
        std::string symbol;
        if (std::regex_match(buffer, match, VersionScriptSymbolRegex) && match[2].str().empty())
            symbol = match[1].str();
        else if (std::regex_match(buffer, match, VersionScriptNamespaceRegex))
            symbol = match[1].str();

        if (std::regex_match(symbol, match, TrailingColonRegex))
            symbol = match[1].str();

        // checkLineForSymbols(buffer, symbol);
        if (!symbol.empty() && symbol[symbol.size() - 1] != ';') {
            std::string relPath = m_currentFileInSourceDir
                    ? std::filesystem::relative(m_currentFile, m_commandLineArgs->sourceDir())
                              .string()
                    : std::filesystem::relative(m_currentFile, m_commandLineArgs->binaryDir())
                              .string();

            std::string versionStringRecord = "    *";
            size_t startPos = 0;
            size_t endPos = 0;
            while (endPos != std::string::npos) {
                endPos = symbol.find("::", startPos);
                size_t length = endPos != std::string::npos ? (endPos - startPos)
                                                            : (symbol.size() - startPos);
                if (length > 0) {
                    std::string symbolPart = symbol.substr(startPos, length);
                    versionStringRecord += std::to_string(symbolPart.size());
                    versionStringRecord += symbolPart;
                }
                startPos = endPos + 2;
            }
            versionStringRecord += "*;";
            if (versionStringRecord.size() < LinkerScriptCommentAlignment)
                versionStringRecord +=
                        std::string(LinkerScriptCommentAlignment - versionStringRecord.size(), ' ');
            versionStringRecord += " # ";
            versionStringRecord += relPath;
            versionStringRecord += ":";
            versionStringRecord += std::to_string(m_currentFileLineNumber);
            versionStringRecord += "\n";
            result.versionScriptContent.push_back(versionStringRecord);
        }
    }

    // The function parses 'headerFile' and collect artifacts that are used at generating step.
    //     'timeStamp' is saved in internal structures to compare it when generating files.
    //     'result' the function output value that stores the result of parsing.
    //     'skipChecks' checks that are not applicable for the header file.
    [[nodiscard]] bool parseHeader(const std::filesystem::path &headerFile,
                                   ParsingResult &result,
                                   unsigned int skipChecks)
    {
        if (m_commandLineArgs->showOnly())
            std::cout << headerFile << " [" << m_commandLineArgs->moduleName() << "]" << std::endl;
        // This regex checks if line contains a macro.
        static const std::regex MacroRegex("^\\s*#.*");

        // The regex's bellow check line for known pragmas:
        //
        //    - 'once' is not allowed in installed headers, so error out.
        //
        //    - 'qt_sync_skip_header_check' avoid any header checks.
        //
        //    - 'qt_sync_stop_processing' stops the header proccesing from a moment when pragma is
        //      found. Important note: All the parsing artifacts were found before this point are
        //      stored for further processing.
        //
        //    - 'qt_sync_suspend_processing' pauses processing and skip lines inside a header until
        //      'qt_sync_resume_processing' is found. 'qt_sync_stop_processing' stops processing if
        //      it's found before the 'qt_sync_resume_processing'.
        //
        //    - 'qt_sync_resume_processing' resumes processing after 'qt_sync_suspend_processing'.
        //
        //    - 'qt_class(<symbol>)' manually declares the 'symbol' that should be used to generate
        //      the CaMeL case header alias.
        //
        //    - 'qt_deprecates([module/]<deprecated header file>[,<major.minor>])' indicates that
        //      this header file replaces the 'deprecated header file'. syncqt will create the
        //      deprecated header file' with the special deprecation content. Pragma optionally
        //      accepts the Qt version where file should be removed. If the current Qt version is
        //      higher than the deprecation version, syncqt displays deprecation warning and skips
        //      generating the deprecated header. If the module is specified and is different from
        //      the one this header file belongs to, syncqt attempts to generate header files
        //      for the specified module. Cross-module deprecation only works within the same repo.
        //      See the 'generateDeprecatedHeaders' function for details.
        //
        //    - 'qt_no_master_include' indicates that syncqt should avoid including this header
        //      files into the module master header file.
        static const std::regex OnceRegex(R"(^#\s*pragma\s+once$)");
        static const std::regex SkipHeaderCheckRegex("^#\\s*pragma qt_sync_skip_header_check$");
        static const std::regex StopProcessingRegex("^#\\s*pragma qt_sync_stop_processing$");
        static const std::regex SuspendProcessingRegex("^#\\s*pragma qt_sync_suspend_processing$");
        static const std::regex ResumeProcessingRegex("^#\\s*pragma qt_sync_resume_processing$");
        static const std::regex ExplixitClassPragmaRegex("^#\\s*pragma qt_class\\(([^\\)]+)\\)$");
        static const std::regex DeprecatesPragmaRegex("^#\\s*pragma qt_deprecates\\(([^\\)]+)\\)$");
        static const std::regex NoMasterIncludePragmaRegex("^#\\s*pragma qt_no_master_include$");

        // This regex checks if header contains 'We mean it' disclaimer. All private headers should
        // contain them.
        static const std::string_view WeMeantItString("We mean it.");

        // The regex's check if the content of header files is wrapped with the Qt namespace macros.
        static const std::regex BeginNamespaceRegex("^QT_BEGIN_NAMESPACE(_[A-Z_]+)?$");
        static const std::regex EndNamespaceRegex("^QT_END_NAMESPACE(_[A-Z_]+)?$");

        // This regex checks if line contains the include macro of the following formats:
        //    - #include <file>
        //    - #include "file"
        //    - #    include <file>
        static const std::regex IncludeRegex("^#\\s*include\\s*[<\"](.+)[>\"]");

        // This regex checks if line contains namespace definition.
        static const std::regex NamespaceRegex("\\s*namespace ([^ ]*)\\s+");

        // This regex checks if line contains the Qt iterator declaration, that need to have
        // CaMel case header alias.
        static const std::regex DeclareIteratorRegex("^ *Q_DECLARE_\\w*ITERATOR\\((\\w+)\\);?$");

        // This regex checks if header file contains the QT_REQUIRE_CONFIG call.
        // The macro argument is used to wrap an include of the header file inside the module master
        // header file with the '#if QT_CONFIG(<feature>)' guard.
        static const std::regex RequireConfigRegex("^ *QT_REQUIRE_CONFIG\\((\\w+)\\);?$");

        // This regex looks for the ELFVERSION tag this is control key-word for the version script
        // content processing.
        // ELFVERSION tag accepts the following values:
        //    - stop - stops the symbols lookup for a version script starting from this line.
        //    - ignore-next - ignores the line followed by the current one.
        //    - ignore - ignores the current line.
        static const std::regex ElfVersionTagRegex(".*ELFVERSION:(stop|ignore-next|ignore).*");

        std::ifstream input(headerFile, std::ifstream::in);
        if (!input.is_open()) {
            std::cerr << "Unable to open " << headerFile << std::endl;
            return false;
        }

        bool hasQtBeginNamespace = false;
        std::string qtBeginNamespace;
        std::string qtEndNamespace;
        bool hasWeMeantIt = false;
        bool isSuspended = false;
        bool isMultiLineComment = false;
        std::size_t bracesDepth = 0;
        std::size_t namespaceCount = 0;
        std::string namespaceString;

        std::smatch match;

        std::string buffer;
        std::string line;
        std::string tmpLine;
        std::size_t linesProcessed = 0;
        int faults = NoChecks;

        const auto error = [&] () -> decltype(auto) {
            return std::cerr << ErrorMessagePreamble << m_currentFileString
                             << ":" << m_currentFileLineNumber << " ";
        };

        // Read file line by line
        while (std::getline(input, tmpLine)) {
            ++m_currentFileLineNumber;
            line.append(tmpLine);
            if (line.empty() || line.at(line.size() - 1) == '\\') {
                continue;
            }
            buffer.clear();
            buffer.reserve(line.size());
            // Optimize processing by looking for a special sequences such as:
            //    - start-end of comments
            //    - start-end of class/structures
            // And avoid processing of the the data inside these blocks.
            for (std::size_t i = 0; i < line.size(); ++i) {
                if (line[i] == '\r')
                    continue;
                if (bracesDepth == namespaceCount) {
                    if (line[i] == '/') {
                        if ((i + 1) < line.size()) {
                            if (line[i + 1] == '*') {
                                isMultiLineComment = true;
                                continue;
                            } else if (line[i + 1] == '/') { // Single line comment
                                if (!(skipChecks & WeMeantItChecks)
                                    && line.find(WeMeantItString) != std::string::npos) {
                                    hasWeMeantIt = true;
                                    continue;
                                }
                                if (m_versionScriptGeneratorState != Stopped
                                    && std::regex_match(line, match, ElfVersionTagRegex)) {
                                    if (match[1].str() == "ignore")
                                        m_versionScriptGeneratorState = Ignore;
                                    else if (match[1].str() == "ignore-next")
                                        m_versionScriptGeneratorState = IgnoreNext;
                                    else if (match[1].str() == "stop")
                                        m_versionScriptGeneratorState = Stopped;
                                }
                                break;
                            }
                        }
                    } else if (line[i] == '*' && (i + 1) < line.size() && line[i + 1] == '/') {
                        ++i;
                        isMultiLineComment = false;
                        continue;
                    }
                }

                if (isMultiLineComment) {
                    if (!(skipChecks & WeMeantItChecks) &&
                        line.find(WeMeantItString) != std::string::npos) {
                        hasWeMeantIt = true;
                        continue;
                    }
                    continue;
                }

                if (line[i] == '{') {
                    if (std::regex_match(buffer, match, NamespaceRegex)) {
                        ++namespaceCount;
                        namespaceString += "::";
                        namespaceString += match[1].str();
                    }
                    ++bracesDepth;
                    continue;
                } else if (line[i] == '}') {
                    if (namespaceCount > 0 && bracesDepth == namespaceCount) {
                        namespaceString.resize(namespaceString.rfind("::"));
                        --namespaceCount;
                    }
                    --bracesDepth;
                } else if (bracesDepth == namespaceCount) {
                    buffer += line[i];
                }
            }
            line.clear();

            scannerDebug() << m_currentFilename << ": " << buffer << std::endl;

            if (m_currentFileType & PrivateHeader) {
                parseVersionScriptContent(buffer, result);
            }

            if (buffer.empty())
                continue;

            ++linesProcessed;

            bool skipSymbols =
                    (m_currentFileType & PrivateHeader) || (m_currentFileType & QpaHeader) || (m_currentFileType & RhiHeader)
                    || (m_currentFileType & SsgHeader);

            // Parse pragmas
            if (std::regex_match(buffer, MacroRegex)) {
                if (std::regex_match(buffer, SkipHeaderCheckRegex)) {
                    skipChecks = AllChecks;
                    faults = NoChecks;
                } else if (std::regex_match(buffer, StopProcessingRegex)) {
                    if (skipChecks == AllChecks)
                        m_headerCheckExceptions.push_back(m_currentFileString);
                    return true;
                } else if (std::regex_match(buffer, SuspendProcessingRegex)) {
                    isSuspended = true;
                } else if (std::regex_match(buffer, ResumeProcessingRegex)) {
                    isSuspended = false;
                } else if (std::regex_match(buffer, match, ExplixitClassPragmaRegex)) {
                    if (!skipSymbols) {
                        updateSymbolDescriptor(match[1].str(), m_currentFilename,
                                               SymbolDescriptor::Pragma);
                    } else {
                        // TODO: warn about skipping symbols that are defined explicitly
                    }
                } else if (std::regex_match(buffer, NoMasterIncludePragmaRegex)) {
                    result.masterInclude = false;
                } else if (std::regex_match(buffer, match, DeprecatesPragmaRegex)) {
                    m_deprecatedHeaders[match[1].str()] =
                            m_commandLineArgs->moduleName() + '/' + m_currentFilename;
                } else if (std::regex_match(buffer, OnceRegex)) {
                    if (!(skipChecks & PragmaOnceChecks)) {
                        faults |= PragmaOnceChecks;
                        error() << "\"#pragma once\" is not allowed in installed header files: "
                                   "https://lists.qt-project.org/pipermail/development/2022-October/043121.html"
                                << std::endl;
                    }
                } else if (std::regex_match(buffer, match, IncludeRegex) && !isSuspended) {
                    if (!(skipChecks & IncludeChecks)) {
                        std::string includedHeader = match[1].str();
                        if (!(skipChecks & PrivateHeaderChecks)
                            && isHeaderPrivate(std::filesystem::path(includedHeader)
                                                       .filename()
                                                       .generic_string())) {
                            faults |= PrivateHeaderChecks;
                            error() << "includes private header " << includedHeader << std::endl;
                        }
                        for (const auto &module : m_commandLineArgs->knownModules()) {
                            std::string suggestedHeader = "Qt" + module + '/' + includedHeader;
                            const std::string suggestedHeaderReversePath = "/../" + suggestedHeader;
                            if (std::filesystem::exists(m_commandLineArgs->includeDir()
                                                        + suggestedHeaderReversePath)
                                || std::filesystem::exists(m_commandLineArgs->installIncludeDir()
                                                           + '/' + suggestedHeader)) {
                                faults |= IncludeChecks;
                                std::cerr << m_warningMessagePreamble << m_currentFileString
                                          << ":" << m_currentFileLineNumber
                                          << " includes " << includedHeader
                                          << " when it should include "
                                          << suggestedHeader << std::endl;
                            }
                        }
                    }
                }
                continue;
            }

            // Logic below this line is affected by the 'qt_sync_suspend_processing' and
            // 'qt_sync_resume_processing' pragmas.
            if (isSuspended)
                continue;

            // Look for the symbols in header file.
            if (!skipSymbols) {
                std::string symbol;
                if (checkLineForSymbols(buffer, symbol)) {
                    if (namespaceCount == 0
                        || std::regex_match(namespaceString,
                                            m_commandLineArgs->publicNamespaceRegex())) {
                        updateSymbolDescriptor(symbol, m_currentFilename,
                                               SymbolDescriptor::Declaration);
                    }
                    continue;
                } else if (std::regex_match(buffer, match, DeclareIteratorRegex)) {
                    std::string iteratorSymbol = match[1].str() + "Iterator";
                    updateSymbolDescriptor(std::string("Q") + iteratorSymbol, m_currentFilename,
                                           SymbolDescriptor::Declaration);
                    updateSymbolDescriptor(std::string("QMutable") + iteratorSymbol,
                                           m_currentFilename, SymbolDescriptor::Declaration);
                    continue;
                } else if (std::regex_match(buffer, match, RequireConfigRegex)) {
                    result.requireConfig = match[1].str();
                    continue;
                }
            }

            // Check for both QT_BEGIN_NAMESPACE and QT_END_NAMESPACE macros are present in the
            // header file.
            if (!(skipChecks & NamespaceChecks)) {
                if (std::regex_match(buffer, match, BeginNamespaceRegex)) {
                    qtBeginNamespace = match[1].str();
                    hasQtBeginNamespace = true;
                } else if (std::regex_match(buffer, match, EndNamespaceRegex)) {
                    qtEndNamespace = match[1].str();
                }
            }
        }
        input.close();

        // Error out if namespace checks are failed.
        if (!(skipChecks & NamespaceChecks)) {
            if (hasQtBeginNamespace) {
                if (qtBeginNamespace != qtEndNamespace) {
                    faults |= NamespaceChecks;
                    std::cerr << m_warningMessagePreamble << m_currentFileString
                              << " the begin namespace macro QT_BEGIN_NAMESPACE" << qtBeginNamespace
                              << " doesn't match the end namespace macro QT_END_NAMESPACE"
                              << qtEndNamespace << std::endl;
                }
            } else {
                faults |= NamespaceChecks;
                std::cerr << m_warningMessagePreamble << m_currentFileString
                          << " does not include QT_BEGIN_NAMESPACE" << std::endl;
            }
        }

        if (!(skipChecks & WeMeantItChecks) && !hasWeMeantIt) {
            faults |= WeMeantItChecks;
            std::cerr << m_warningMessagePreamble << m_currentFileString
                      << " does not have the \"We mean it.\" warning"
                      << std::endl;
        }

        scannerDebug() << "linesTotal: " << m_currentFileLineNumber
                       << " linesProcessed: " << linesProcessed << std::endl;

        if (skipChecks == AllChecks)
            m_headerCheckExceptions.push_back(m_currentFileString);

        // Exit with an error if any of critical checks are present.
        return !(faults & m_criticalChecks);
    }

    // The function checks if line contains the symbol that needs to have a CaMeL-style alias.
    [[nodiscard]] bool checkLineForSymbols(const std::string &line, std::string &symbol)
    {
        scannerDebug() << "checkLineForSymbols: " << line << std::endl;

        // This regex checks if line contains class or structure declaration like:
        //     - <class|stuct> StructName
        //     - template <> class ClassName
        //     - class ClassName : [public|protected|private] BaseClassName
        //     - class ClassName [QT_TEXT_STREAM_FINAL|Q_DECL_FINAL|final|sealed]
        // And possible combinations of the above variants.
        static const std::regex ClassRegex(
                "^ *(template *<.*> *)?(class|struct +)([^<>:]*\\s+)?"       // Preceding part
                "((?!Q[A-Z_0-9]*_FINAL|final|sealed)Q[a-zA-Z0-9_]+)"         // Actual symbol
                "(\\s+Q[A-Z_0-9]*_FINAL|\\s+final|\\s+sealed)?\\s*(:|$).*"); // Trailing part

        // This regex checks if line contains function pointer typedef declaration like:
        //     - typedef void (* QFunctionPointerType)(int, char);
        static const std::regex FunctionPointerRegex(
                "^ *typedef *.*\\(\\*(Q[^\\)]+)\\)\\(.*\\); *");

        // This regex checks if line contains class or structure typedef declaration like:
        //     - typedef AnySymbol<char> QAnySymbolType;
        static const std::regex TypedefRegex("^ *typedef\\s+(.*)\\s+(Q\\w+); *$");

        std::smatch match;
        if (std::regex_match(line, match, FunctionPointerRegex)) {
            symbol = match[1].str();
        } else if (std::regex_match(line, match, TypedefRegex)) {
            symbol = match[2].str();
        } else if (std::regex_match(line, match, ClassRegex)) {
            symbol = match[4].str();
        } else {
            return false;
        }
        return !symbol.empty();
    }

    [[nodiscard]] bool isHeaderQpa(const std::string &headerFileName)
    {
        return std::regex_match(headerFileName, m_commandLineArgs->qpaHeadersRegex());
    }

    [[nodiscard]] bool isHeaderRhi(const std::string &headerFileName)
    {
        return std::regex_match(headerFileName, m_commandLineArgs->rhiHeadersRegex());
    }

    [[nodiscard]] bool isHeaderSsg(const std::string &headerFileName)
    {
        return std::regex_match(headerFileName, m_commandLineArgs->ssgHeadersRegex());
    }

    [[nodiscard]] bool isHeaderPrivate(const std::string &headerFile)
    {
        return std::regex_match(headerFile, m_commandLineArgs->privateHeadersRegex());
    }

    [[nodiscard]] bool isHeaderPCH(const std::string &headerFilename)
    {
        static const std::string pchSuffix("_pch.h");
        return headerFilename.find(pchSuffix, headerFilename.size() - pchSuffix.size())
                != std::string::npos;
    }

    [[nodiscard]] bool isHeader(const std::filesystem::path &path)
    {
        return path.extension().string() == ".h";
    }

    [[nodiscard]] bool isDocFileHeuristic(const std::string &headerFilePath)
    {
        return headerFilePath.find("/doc/") != std::string::npos;
    }

    [[nodiscard]] bool isHeaderGenerated(const std::string &header)
    {
        return m_commandLineArgs->generatedHeaders().find(header)
                != m_commandLineArgs->generatedHeaders().end();
    }

    [[nodiscard]] bool generateQtCamelCaseFileIfContentChanged(const std::string &outputFilePath,
                                                               const std::string &aliasedFilePath);

    [[nodiscard]] bool generateAliasedHeaderFileIfTimestampChanged(
            const std::string &outputFilePath, const std::string &aliasedFilePath,
            const FileStamp &originalStamp = FileStamp::clock::now());

    bool writeIfDifferent(const std::string &outputFile, const std::string &buffer);

    [[nodiscard]] bool generateMasterHeader()
    {
        if (m_masterHeaderContents.empty())
            return true;

        std::string outputFile =
                m_commandLineArgs->includeDir() + '/' + m_commandLineArgs->moduleName();

        std::string moduleUpper = utils::asciiToUpper(m_commandLineArgs->moduleName());
        std::stringstream buffer;
        buffer << "#ifndef QT_" << moduleUpper << "_MODULE_H\n"
               << "#define QT_" << moduleUpper << "_MODULE_H\n"
               << "#include <" << m_commandLineArgs->moduleName() << "/"
               << m_commandLineArgs->moduleName() << "Depends>\n";
        for (const auto &headerContents : m_masterHeaderContents) {
            if (!headerContents.second.empty()) {
                buffer << "#if QT_CONFIG(" << headerContents.second << ")\n"
                       << "#include \"" << headerContents.first << "\"\n"
                       << "#endif\n";
            } else {
                buffer << "#include \"" << headerContents.first << "\"\n";
            }
        }
        buffer << "#endif\n";

        m_producedHeaders.insert(m_commandLineArgs->moduleName());
        return writeIfDifferent(outputFile, buffer.str());
    }

    [[nodiscard]] bool generateVersionHeader(const std::string &outputFile)
    {
        std::string moduleNameUpper = utils::asciiToUpper( m_commandLineArgs->moduleName());

        std::stringstream buffer;
        buffer << "/* This file was generated by syncqt. */\n"
               << "#ifndef QT_" << moduleNameUpper << "_VERSION_H\n"
               << "#define QT_" << moduleNameUpper << "_VERSION_H\n\n"
               << "#define " << moduleNameUpper << "_VERSION_STR \"" << QT_VERSION_STR << "\"\n\n"
               << "#define " << moduleNameUpper << "_VERSION "
               << "0x0" << QT_VERSION_MAJOR << "0" << QT_VERSION_MINOR << "0" << QT_VERSION_PATCH
               << "\n\n"
               << "#endif // QT_" << moduleNameUpper << "_VERSION_H\n";

        return writeIfDifferent(outputFile, buffer.str());
    }

    [[nodiscard]] bool generateDeprecatedHeaders()
    {
        static std::regex cIdentifierSymbolsRegex("[^a-zA-Z0-9_]");
        const std::string guard_base = "DEPRECATED_HEADER_" + m_commandLineArgs->moduleName();
        bool result = true;
        for (auto it = m_deprecatedHeaders.begin(); it != m_deprecatedHeaders.end(); ++it) {
            const std::string &descriptor = it->first;
            const std::string &replacement = it->second;

            const auto separatorPos = descriptor.find(',');
            std::string headerPath = descriptor.substr(0, separatorPos);
            std::string versionDisclaimer;
            if (separatorPos != std::string::npos) {
                std::string version = descriptor.substr(separatorPos + 1);
                versionDisclaimer = " and will be removed in Qt " + version;
                int minor = 0;
                int major = 0;
                if (!utils::parseVersion(version, major, minor)) {
                    std::cerr << ErrorMessagePreamble
                              << "Invalid version format specified for the deprecated header file "
                              << headerPath << ": '" << version
                              << "'. Expected format: 'major.minor'.\n";
                    result = false;
                    continue;
                }

                if (QT_VERSION_MAJOR > major
                    || (QT_VERSION_MAJOR == major && QT_VERSION_MINOR >= minor)) {
                    std::cerr << WarningMessagePreamble << headerPath
                              << " is marked as deprecated and will not be generated in Qt "
                              << QT_VERSION_STR
                              << ". The respective qt_deprecates pragma needs to be removed.\n";
                    continue;
                }
            }

            const auto moduleSeparatorPos = headerPath.find('/');
            std::string headerName = moduleSeparatorPos != std::string::npos
                    ? headerPath.substr(moduleSeparatorPos + 1)
                    : headerPath;
            const std::string moduleName = moduleSeparatorPos != std::string::npos
                    ? headerPath.substr(0, moduleSeparatorPos)
                    : m_commandLineArgs->moduleName();

            bool isCrossModuleDeprecation = moduleName != m_commandLineArgs->moduleName();

            std::string qualifiedHeaderName =
                    std::regex_replace(headerName, cIdentifierSymbolsRegex, "_");
            std::string guard = guard_base + "_" + qualifiedHeaderName;
            std::string warningText = "Header <" + moduleName + "/" + headerName + "> is deprecated"
                    + versionDisclaimer + ". Please include <" + replacement + "> instead.";
            std::stringstream buffer;
            buffer << "#ifndef " << guard << "\n"
                   << "#define " << guard << "\n"
                   << "#if defined(__GNUC__)\n"
                   << "#  warning " << warningText << "\n"
                   << "#elif defined(_MSC_VER)\n"
                   << "#  pragma message (\"" << warningText << "\")\n"
                   << "#endif\n"
                   << "#include <" << replacement << ">\n"
                   << "#endif\n";

            const std::string outputDir = isCrossModuleDeprecation
                    ? m_commandLineArgs->includeDir() + "/../" + moduleName
                    : m_commandLineArgs->includeDir();
            writeIfDifferent(outputDir + '/' + headerName, buffer.str());

            // Add header file to staging installation directory for cross-module deprecation case.
            if (isCrossModuleDeprecation) {
                const std::string stagingDir = outputDir + "/.syncqt_staging/";
                writeIfDifferent(stagingDir + headerName, buffer.str());
            }
            m_producedHeaders.insert(headerName);
        }
        return result;
    }

    [[nodiscard]] bool generateHeaderCheckExceptions()
    {
        std::stringstream buffer;
        for (const auto &header : m_headerCheckExceptions)
            buffer << header << ";";
        return writeIfDifferent(m_commandLineArgs->binaryDir() + '/'
                                        + m_commandLineArgs->moduleName()
                                        + "_header_check_exceptions",
                                buffer.str());
    }

    [[nodiscard]] bool generateLinkerVersionScript()
    {
        std::stringstream buffer;
        for (const auto &content : m_versionScriptContents)
            buffer << content;
        return writeIfDifferent(m_commandLineArgs->versionScriptFile(), buffer.str());
    }

    bool updateOrCopy(const std::filesystem::path &src, const std::filesystem::path &dst) noexcept;
    void updateSymbolDescriptor(const std::string &symbol, const std::string &file,
                                SymbolDescriptor::SourceType type);
};

// The function updates information about the symbol:
//    - The path and modification time of the file where the symbol was found.
//    - The source of finding
// Also displays a short info about a symbol in show only mode.
void SyncScanner::updateSymbolDescriptor(const std::string &symbol, const std::string &file,
                                         SymbolDescriptor::SourceType type)
{
    if (m_commandLineArgs->showOnly())
        std::cout << "    SYMBOL: " << symbol << std::endl;
    m_symbols[symbol].update(file, type);
}

[[nodiscard]] std::filesystem::path
SyncScanner::makeHeaderAbsolute(const std::string &filename) const
{
    if (std::filesystem::path(filename).is_relative())
        return utils::normilizedPath(m_commandLineArgs->sourceDir() + '/' + filename);

    return utils::normilizedPath(filename);
}

bool SyncScanner::updateOrCopy(const std::filesystem::path &src,
                               const std::filesystem::path &dst) noexcept
{
    if (m_commandLineArgs->showOnly())
        return true;

    if (src == dst) {
        std::cout << "Source and destination paths are same when copying " << src.string()
                  << ". Skipping." << std::endl;
        return true;
    }

    std::error_code ec;
    std::filesystem::copy(src, dst, std::filesystem::copy_options::update_existing, ec);
    if (ec) {
        ec.clear();
        std::filesystem::remove(dst, ec);
        if (ec) {
            // On some file systems(e.g. vboxfs) the std::filesystem::copy doesn't support
            // std::filesystem::copy_options::overwrite_existing remove file first and then copy.
            std::cerr << "Unable to remove file: " << src << " to " << dst << " error: ("
                << ec.value() << ")" << ec.message() << std::endl;
            return false;
        }

        std::filesystem::copy(src, dst, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            std::cerr << "Unable to copy file: " << src << " to " << dst << " error: ("
                      << ec.value() << ")" << ec.message() << std::endl;
            return false;
        }
    }
    return true;
}

// The function generates Qt CaMeL-case files.
// CaMeL-case files can have timestamp that is the same as or newer than timestamp of file that
// supposed to included there. It's not an issue when we generate regular aliases because the
// content of aliases is always the same, but we only want to "touch" them when content of original
// is changed.
bool SyncScanner::generateQtCamelCaseFileIfContentChanged(const std::string &outputFilePath,
                                                          const std::string &aliasedFilePath)
{
    if (m_commandLineArgs->showOnly())
        return true;

    std::string buffer = "#include \"";
    buffer += aliasedFilePath;
    buffer += "\" // IWYU pragma: export\n";

    return writeIfDifferent(outputFilePath, buffer);
}

// The function generates aliases for files in source tree. Since the content of these aliases is
// always same, it's ok to check only timestamp and touch files in case if stamp of original is
// newer than the timestamp of an alias.
bool SyncScanner::generateAliasedHeaderFileIfTimestampChanged(const std::string &outputFilePath,
                                                              const std::string &aliasedFilePath,
                                                              const FileStamp &originalStamp)
{
    if (m_commandLineArgs->showOnly())
        return true;

    if (std::filesystem::exists({ outputFilePath })
        && std::filesystem::last_write_time({ outputFilePath }) >= originalStamp) {
        return true;
    }
    scannerDebug() << "Rewrite " << outputFilePath << std::endl;

    std::ofstream ofs;
    ofs.open(outputFilePath, std::ofstream::out | std::ofstream::trunc);
    if (!ofs.is_open()) {
        std::cerr << "Unable to write header file alias: " << outputFilePath << std::endl;
        return false;
    }
    ofs << "#include \"" << aliasedFilePath << "\" // IWYU pragma: export\n";
    ofs.close();
    return true;
}

bool SyncScanner::writeIfDifferent(const std::string &outputFile, const std::string &buffer)
{
    if (m_commandLineArgs->showOnly())
        return true;

    static const std::streamsize bufferSize = 1025;
    bool differs = false;
    std::filesystem::path outputFilePath(outputFile);

    std::string outputDirectory = outputFilePath.parent_path().string();

    if (!utils::createDirectories(outputDirectory, "Unable to create output directory"))
        return false;

    auto expectedSize = buffer.size();
#ifdef _WINDOWS
    // File on disk has \r\n instead of just \n
    expectedSize += std::count(buffer.begin(), buffer.end(), '\n');
#endif

    if (std::filesystem::exists(outputFilePath)
        && expectedSize == std::filesystem::file_size(outputFilePath)) {
        char rdBuffer[bufferSize];
        memset(rdBuffer, 0, bufferSize);

        std::ifstream ifs(outputFile, std::fstream::in);
        if (!ifs.is_open()) {
            std::cerr << "Unable to open " << outputFile << " for comparison." << std::endl;
            return false;
        }
        std::streamsize currentPos = 0;

        std::size_t bytesRead = 0;
        do {
            ifs.read(rdBuffer, bufferSize - 1); // Read by 1K
            bytesRead = ifs.gcount();
            if (buffer.compare(currentPos, bytesRead, rdBuffer) != 0) {
                differs = true;
                break;
            }
            currentPos += bytesRead;
            memset(rdBuffer, 0, bufferSize);
        } while (bytesRead > 0);

        ifs.close();
    } else {
        differs = true;
    }

    scannerDebug() << "Update: " << outputFile << " " << differs << std::endl;
    if (differs) {
        std::ofstream ofs;
        ofs.open(outputFilePath, std::fstream::out | std::ofstream::trunc);
        if (!ofs.is_open()) {
            std::cerr << "Unable to write header content to " << outputFilePath << std::endl;
            return false;
        }
        ofs << buffer;

        ofs.close();
    }
    return true;
}

int main(int argc, char *argv[])
{
    CommandLineOptions options(argc, argv);
    if (!options.isValid())
        return InvalidArguments;

    if (options.printHelpOnly()) {
        options.printHelp();
        return NoError;
    }

    SyncScanner scanner = SyncScanner(&options);
    return scanner.sync();
}
