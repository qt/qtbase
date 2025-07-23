// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "utils.h"

#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QScopedPointer>
#include <QtCore/QScopedArrayPointer>
#include <QtCore/QStandardPaths>
#if defined(Q_OS_WIN)
#  include <QtCore/qt_windows.h>
#  include <QtCore/private/qsystemerror_p.h>
#  include <shlwapi.h>
#  include <delayimp.h>
#endif  // Q_OS_WIN

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

int optVerboseLevel = 1;

bool isBuildDirectory(Platform platform, const QString &dirName)
{
    return (platform.testFlag(Msvc) || platform.testFlag(ClangMsvc))
        && (dirName == "debug"_L1 || dirName == "release"_L1);
}

// Create a symbolic link by changing to the source directory to make sure the
// link uses relative paths only (QFile::link() otherwise uses the absolute path).
bool createSymbolicLink(const QFileInfo &source, const QString &target, QString *errorMessage)
{
    const QString oldDirectory = QDir::currentPath();
    if (!QDir::setCurrent(source.absolutePath())) {
        *errorMessage = QStringLiteral("Unable to change to directory %1.").arg(QDir::toNativeSeparators(source.absolutePath()));
        return false;
    }
    QFile file(source.fileName());
    const bool success = file.link(target);
    QDir::setCurrent(oldDirectory);
    if (!success) {
        *errorMessage = QString::fromLatin1("Failed to create symbolic link %1 -> %2: %3")
                        .arg(QDir::toNativeSeparators(source.absoluteFilePath()),
                             QDir::toNativeSeparators(target), file.errorString());
        return false;
    }
    return true;
}

bool createDirectory(const QString &directory, QString *errorMessage, bool dryRun)
{
    const QFileInfo fi(directory);
    if (fi.isDir())
        return true;
    if (fi.exists()) {
        *errorMessage = QString::fromLatin1("%1 already exists and is not a directory.").
                        arg(QDir::toNativeSeparators(directory));
        return false;
    }
    if (optVerboseLevel)
        std::wcout << "Creating " << QDir::toNativeSeparators(directory) << "...\n";
    if (!dryRun) {
        QDir dir;
        if (!dir.mkpath(directory)) {
            *errorMessage = QString::fromLatin1("Could not create directory %1.")
                                    .arg(QDir::toNativeSeparators(directory));
            return false;
        }
    }
    return true;
}

// Find shared libraries matching debug/Platform in a directory, return relative names.
QStringList findSharedLibraries(const QDir &directory, Platform platform,
                                DebugMatchMode debugMatchMode,
                                const QString &prefix)
{
    QString nameFilter = prefix;
    if (nameFilter.isEmpty())
        nameFilter += u'*';
    if (debugMatchMode == MatchDebug && platformHasDebugSuffix(platform))
        nameFilter += u'd';
    nameFilter += sharedLibrarySuffix();
    QStringList result;
    QString errorMessage;
    const QFileInfoList &dlls = directory.entryInfoList(QStringList(nameFilter), QDir::Files);
    for (const QFileInfo &dllFi : dlls) {
        const QString dllPath = dllFi.absoluteFilePath();
        bool matches = true;
        if (debugMatchMode != MatchDebugOrRelease && (platform & WindowsBased)) {
            bool debugDll;
            if (readPeExecutable(dllPath, &errorMessage, 0, 0, &debugDll,
                                 (platform == WindowsDesktopMinGW))) {
                matches = debugDll == (debugMatchMode == MatchDebug);
            } else {
                std::wcerr << "Warning: Unable to read " << QDir::toNativeSeparators(dllPath)
                           << ": " << errorMessage;
            }
        } // Windows
        if (matches)
            result += dllFi.fileName();
    } // for
    return result;
}

// Case-Normalize file name via GetShortPathNameW()/GetLongPathNameW()
QString normalizeFileName(const QString &name)
{
    wchar_t shortBuffer[MAX_PATH];
    const QString nativeFileName = QDir::toNativeSeparators(name);
    if (!GetShortPathNameW(reinterpret_cast<LPCWSTR>(nativeFileName.utf16()), shortBuffer, MAX_PATH))
        return name;
    wchar_t result[MAX_PATH];
    if (!GetLongPathNameW(shortBuffer, result, MAX_PATH))
        return name;
    return QDir::fromNativeSeparators(QString::fromWCharArray(result));
}

static inline void appendToCommandLine(const QString &argument, QString *commandLine)
{
    const bool needsQuote = argument.contains(u' ');
    if (!commandLine->isEmpty())
        commandLine->append(u' ');
    if (needsQuote)
        commandLine->append(u'"');
    commandLine->append(argument);
    if (needsQuote)
        commandLine->append(u'"');
}

bool runProcess(const QString &binary, const QStringList &args,
                const QString &workingDirectory,
                unsigned long *exitCode, QByteArray *stdOut, QByteArray *stdErr,
                QString *errorMessage, int timeout)
{
    if (exitCode)
        *exitCode = 0;

    QProcess process;
    process.setProgram(binary);
    process.setArguments(args);
    process.setWorkingDirectory(workingDirectory);

    // Output the command if requested.
    if (optVerboseLevel > 1) {
        QString commandLine;
        appendToCommandLine(binary, &commandLine);
        for (const QString &a : args)
            appendToCommandLine(a, &commandLine);
        std::wcout << "Running: " << commandLine << '\n';
    }

    process.start();
    if (!process.waitForStarted() || !process.waitForFinished(timeout)) {
        if (errorMessage)
            *errorMessage = process.errorString();
        return false;
    }

    if (stdOut)
        *stdOut = process.readAllStandardOutput();
    if (stdErr)
        *stdErr = process.readAllStandardError();

    return true;
}

// Find a file in the path using ShellAPI. This can be used to locate DLLs which
// QStandardPaths cannot do.
QString findInPath(const QString &file)
{
#if defined(Q_OS_WIN)
    if (file.size() < MAX_PATH -  1) {
        wchar_t buffer[MAX_PATH];
        file.toWCharArray(buffer);
        buffer[file.size()] = 0;
        if (PathFindOnPath(buffer, NULL))
            return QDir::cleanPath(QString::fromWCharArray(buffer));
    }
    return QString();
#else // Q_OS_WIN
    return QStandardPaths::findExecutable(file);
#endif // !Q_OS_WIN
}

const char *qmakeInfixKey = "QT_INFIX";

QMap<QString, QString> queryQtPaths(const QString &qtpathsBinary, QString *errorMessage)
{
    const QString binary = !qtpathsBinary.isEmpty() ? qtpathsBinary : QStringLiteral("qtpaths");
    const QString colonSpace = QStringLiteral(": ");
    QByteArray stdOut;
    QByteArray stdErr;
    unsigned long exitCode = 0;
    if (!runProcess(binary, QStringList(QStringLiteral("-query")), QString(), &exitCode, &stdOut,
                    &stdErr, errorMessage)) {
        *errorMessage = QStringLiteral("Error running binary ") + binary + colonSpace + *errorMessage;
        return QMap<QString, QString>();
    }
    if (exitCode) {
        *errorMessage = binary + QStringLiteral(" returns ") + QString::number(exitCode)
            + colonSpace + QString::fromLocal8Bit(stdErr);
        return QMap<QString, QString>();
    }
    const QString output = QString::fromLocal8Bit(stdOut).trimmed().remove(u'\r');
    QMap<QString, QString> result;
    const qsizetype size = output.size();
    for (qsizetype pos = 0; pos < size; ) {
        const qsizetype colonPos = output.indexOf(u':', pos);
        if (colonPos < 0)
            break;
        qsizetype endPos = output.indexOf(u'\n', colonPos + 1);
        if (endPos < 0)
            endPos = size;
        const QString key = output.mid(pos, colonPos - pos);
        const QString value = output.mid(colonPos + 1, endPos - colonPos - 1);
        result.insert(key, value);
        pos = endPos + 1;
    }
    QFile qconfigPriFile(result.value(QStringLiteral("QT_HOST_DATA")) + QStringLiteral("/mkspecs/qconfig.pri"));
    if (qconfigPriFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray lineArray;
        while (qconfigPriFile.readLineInto(&lineArray)) {
            QByteArrayView line = QByteArrayView(lineArray);
            if (line.startsWith("QT_LIBINFIX")) {
                const int pos = line.indexOf('=');
                if (pos >= 0) {
                    const QString infix = QString::fromUtf8(line.right(line.size() - pos - 1).trimmed());
                    if (!infix.isEmpty())
                        result.insert(QLatin1StringView(qmakeInfixKey), infix);
                }
                break;
            }
        }
    } else {
        std::wcerr << "Warning: Unable to read " << QDir::toNativeSeparators(qconfigPriFile.fileName())
            << colonSpace << qconfigPriFile.errorString()<< '\n';
    }
    return result;
}

// Update a file or directory.
bool updateFile(const QString &sourceFileName, const QStringList &nameFilters,
                const QString &targetDirectory, unsigned flags, JsonOutput *json, QString *errorMessage)
{
    const QFileInfo sourceFileInfo(sourceFileName);
    const QString targetFileName = targetDirectory + u'/' + sourceFileInfo.fileName();
    if (optVerboseLevel > 1)
        std::wcout << "Checking " << sourceFileName << ", " << targetFileName<< '\n';

    if (!sourceFileInfo.exists()) {
        *errorMessage = QString::fromLatin1("%1 does not exist.").arg(QDir::toNativeSeparators(sourceFileName));
        return false;
    }

    if (sourceFileInfo.isSymLink()) {
        *errorMessage = QString::fromLatin1("Symbolic links are not supported (%1).")
                        .arg(QDir::toNativeSeparators(sourceFileName));
        return false;
    }

    const QFileInfo targetFileInfo(targetFileName);

    if (sourceFileInfo.isDir()) {
        if (targetFileInfo.exists()) {
            if (!targetFileInfo.isDir()) {
                *errorMessage = QString::fromLatin1("%1 already exists and is not a directory.")
                                .arg(QDir::toNativeSeparators(targetFileName));
                return false;
            } // Not a directory.
        } else { // exists.
            QDir d(targetDirectory);
            if (optVerboseLevel)
                std::wcout << "Creating " << QDir::toNativeSeparators(targetFileName) << ".\n";
            if (!(flags & SkipUpdateFile) && !d.mkdir(sourceFileInfo.fileName())) {
                *errorMessage = QString::fromLatin1("Cannot create directory %1 under %2.")
                                .arg(sourceFileInfo.fileName(), QDir::toNativeSeparators(targetDirectory));
                return false;
            }
        }
        // Recurse into directory
        QDir dir(sourceFileName);
        const QFileInfoList allEntries = dir.entryInfoList(nameFilters, QDir::Files)
            + dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entryFi : allEntries) {
            if (!updateFile(entryFi.absoluteFilePath(), nameFilters, targetFileName, flags, json, errorMessage))
                return false;
        }
        return true;
    } // Source is directory.

    if (targetFileInfo.exists()) {
        if (!(flags & ForceUpdateFile)
            && targetFileInfo.lastModified() >= sourceFileInfo.lastModified()) {
            if (optVerboseLevel)
                std::wcout << sourceFileInfo.fileName() << " is up to date.\n";
            if (json)
                json->addFile(sourceFileName, targetDirectory);
            return true;
        }
        QFile targetFile(targetFileName);
        if (!(flags & SkipUpdateFile) && !targetFile.remove()) {
            *errorMessage = QString::fromLatin1("Cannot remove existing file %1: %2")
                            .arg(QDir::toNativeSeparators(targetFileName), targetFile.errorString());
            return false;
        }
    } // target exists
    QFile file(sourceFileName);
    if (optVerboseLevel)
        std::wcout << "Updating " << sourceFileInfo.fileName() << ".\n";
    if (!(flags & SkipUpdateFile) && !file.copy(targetFileName)) {
        *errorMessage = QString::fromLatin1("Cannot copy %1 to %2: %3")
                .arg(QDir::toNativeSeparators(sourceFileName),
                     QDir::toNativeSeparators(targetFileName),
                     file.errorString());
        return false;
    }
    if (json)
        json->addFile(sourceFileName, targetDirectory);
    return true;
}

#ifdef Q_OS_WIN

static inline QString stringFromRvaPtr(const void *rvaPtr)
{
    return QString::fromLocal8Bit(static_cast<const char *>(rvaPtr));
}

// Helper for reading out PE executable files: Find a section header for an RVA
// (IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32).
template <class ImageNtHeader>
const IMAGE_SECTION_HEADER *findSectionHeader(DWORD rva, const ImageNtHeader *nTHeader)
{
    const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION(nTHeader);
    const IMAGE_SECTION_HEADER *sectionEnd = section + nTHeader->FileHeader.NumberOfSections;
    for ( ; section < sectionEnd; ++section)
        if (rva >= section->VirtualAddress && rva < (section->VirtualAddress + section->Misc.VirtualSize))
                return section;
    return 0;
}

// Helper for reading out PE executable files: convert RVA to pointer (IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32).
template <class ImageNtHeader>
inline const void *rvaToPtr(DWORD rva, const ImageNtHeader *nTHeader, const void *imageBase)
{
    const IMAGE_SECTION_HEADER *sectionHdr = findSectionHeader(rva, nTHeader);
    if (!sectionHdr)
        return 0;
    const DWORD delta = sectionHdr->VirtualAddress - sectionHdr->PointerToRawData;
    return static_cast<const char *>(imageBase) + rva - delta;
}

// Helper for reading out PE executable files: return word size of a IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32
template <class ImageNtHeader>
inline unsigned ntHeaderWordSize(const ImageNtHeader *header)
{
    // defines IMAGE_NT_OPTIONAL_HDR32_MAGIC, IMAGE_NT_OPTIONAL_HDR64_MAGIC
    enum { imageNtOptionlHeader32Magic = 0x10b, imageNtOptionlHeader64Magic = 0x20b };
    if (header->OptionalHeader.Magic == imageNtOptionlHeader32Magic)
        return 32;
    if (header->OptionalHeader.Magic == imageNtOptionlHeader64Magic)
        return 64;
    return 0;
}

// Helper for reading out PE executable files: Retrieve the NT image header of an
// executable via the legacy DOS header.
static IMAGE_NT_HEADERS *getNtHeader(void *fileMemory, QString *errorMessage)
{
    IMAGE_DOS_HEADER *dosHeader = static_cast<PIMAGE_DOS_HEADER>(fileMemory);
    // Check DOS header consistency
    if (IsBadReadPtr(dosHeader, sizeof(IMAGE_DOS_HEADER))
        || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        *errorMessage = QString::fromLatin1("DOS header check failed.");
        return 0;
    }
    // Retrieve NT header
    char *ntHeaderC = static_cast<char *>(fileMemory) + dosHeader->e_lfanew;
    IMAGE_NT_HEADERS *ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS *>(ntHeaderC);
    // check NT header consistency
    if (IsBadReadPtr(ntHeaders, sizeof(ntHeaders->Signature))
        || ntHeaders->Signature != IMAGE_NT_SIGNATURE
        || IsBadReadPtr(&ntHeaders->FileHeader, sizeof(IMAGE_FILE_HEADER))) {
        *errorMessage = QString::fromLatin1("NT header check failed.");
        return 0;
    }
    // Check magic
    if (!ntHeaderWordSize(ntHeaders)) {
        *errorMessage = QString::fromLatin1("NT header check failed; magic %1 is invalid.").
                        arg(ntHeaders->OptionalHeader.Magic);
        return 0;
    }
    // Check section headers
    IMAGE_SECTION_HEADER *sectionHeaders = IMAGE_FIRST_SECTION(ntHeaders);
    if (IsBadReadPtr(sectionHeaders, ntHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))) {
        *errorMessage = QString::fromLatin1("NT header section header check failed.");
        return 0;
    }
    return ntHeaders;
}

// Helper for reading out PE executable files: Read out import sections from
// IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32.
template <class ImageNtHeader>
inline QStringList readImportSections(const ImageNtHeader *ntHeaders, const void *base, QString *errorMessage)
{
    // Get import directory entry RVA and read out
    const DWORD importsStartRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!importsStartRVA) {
        *errorMessage = QString::fromLatin1("Failed to find IMAGE_DIRECTORY_ENTRY_IMPORT entry.");
        return QStringList();
    }
    const IMAGE_IMPORT_DESCRIPTOR *importDesc = static_cast<const IMAGE_IMPORT_DESCRIPTOR *>(rvaToPtr(importsStartRVA, ntHeaders, base));
    if (!importDesc) {
        *errorMessage = QString::fromLatin1("Failed to find IMAGE_IMPORT_DESCRIPTOR entry.");
        return QStringList();
    }
    QStringList result;
    for ( ; importDesc->Name; ++importDesc)
        result.push_back(stringFromRvaPtr(rvaToPtr(importDesc->Name, ntHeaders, base)));

    // Read delay-loaded DLLs, see http://msdn.microsoft.com/en-us/magazine/cc301808.aspx .
    // Check on grAttr bit 1 whether this is the format using RVA's > VS 6
    if (const DWORD delayedImportsStartRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress) {
        const ImgDelayDescr *delayedImportDesc = static_cast<const ImgDelayDescr *>(rvaToPtr(delayedImportsStartRVA, ntHeaders, base));
        for ( ; delayedImportDesc->rvaDLLName && (delayedImportDesc->grAttrs & 1); ++delayedImportDesc)
            result.push_back(stringFromRvaPtr(rvaToPtr(delayedImportDesc->rvaDLLName, ntHeaders, base)));
    }

    return result;
}

// Check for MSCV runtime (MSVCP90D.dll/MSVCP90.dll, MSVCP120D.dll/MSVCP120.dll,
// VCRUNTIME140D.DLL/VCRUNTIME140.DLL (VS2015) or msvcp120d_app.dll/msvcp120_app.dll).
enum MsvcDebugRuntimeResult { MsvcDebugRuntime, MsvcReleaseRuntime, NoMsvcRuntime };

static inline MsvcDebugRuntimeResult checkMsvcDebugRuntime(const QStringList &dependentLibraries)
{
    for (const QString &lib : dependentLibraries) {
        qsizetype pos = 0;
        if (lib.startsWith("MSVCR"_L1, Qt::CaseInsensitive)
            || lib.startsWith("MSVCP"_L1, Qt::CaseInsensitive)
            || lib.startsWith("VCRUNTIME"_L1, Qt::CaseInsensitive)
            || lib.startsWith("VCCORLIB"_L1, Qt::CaseInsensitive)
            || lib.startsWith("CONCRT"_L1, Qt::CaseInsensitive)
            || lib.startsWith("UCRTBASE"_L1, Qt::CaseInsensitive)) {
            qsizetype lastDotPos = lib.lastIndexOf(u'.');
            pos = -1 == lastDotPos ? 0 : lastDotPos - 1;
        }

        if (pos > 0) {
            const auto removeExtraSuffix = [&lib, &pos](const QString &suffix) -> void {
                if (lib.contains(suffix, Qt::CaseInsensitive))
                    pos -= suffix.size();
            };
            removeExtraSuffix("_app"_L1);
            removeExtraSuffix("_atomic_wait"_L1);
            removeExtraSuffix("_codecvt_ids"_L1);
        }

        if (pos)
            return lib.at(pos).toLower() == u'd' ? MsvcDebugRuntime : MsvcReleaseRuntime;
    }
    return NoMsvcRuntime;
}

template <class ImageNtHeader>
inline QStringList determineDependentLibs(const ImageNtHeader *nth, const void *fileMemory,
                                           QString *errorMessage)
{
    return readImportSections(nth, fileMemory, errorMessage);
}

template <class ImageNtHeader>
inline bool determineDebug(const ImageNtHeader *nth, const void *fileMemory,
                           QStringList *dependentLibrariesIn, QString *errorMessage)
{
    if (nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
        return false;

    const QStringList dependentLibraries = dependentLibrariesIn != nullptr ?
                *dependentLibrariesIn :
                determineDependentLibs(nth, fileMemory, errorMessage);

    const bool hasDebugEntry = nth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    // When an MSVC debug entry is present, check whether the debug runtime
    // is actually used to detect -release / -force-debug-info builds.
    const MsvcDebugRuntimeResult msvcrt = checkMsvcDebugRuntime(dependentLibraries);
    if (msvcrt == NoMsvcRuntime)
        return hasDebugEntry;
    else
        return hasDebugEntry && msvcrt == MsvcDebugRuntime;
}

template <class ImageNtHeader>
inline void determineDebugAndDependentLibs(const ImageNtHeader *nth, const void *fileMemory,
                                           QStringList *dependentLibrariesIn,
                                           bool *isDebugIn, QString *errorMessage)
{
    if (dependentLibrariesIn)
        *dependentLibrariesIn = determineDependentLibs(nth, fileMemory, errorMessage);

    if (isDebugIn)
        *isDebugIn = determineDebug(nth, fileMemory, dependentLibrariesIn, errorMessage);
}

// Read a PE executable and determine dependent libraries, word size
// and debug flags.
bool readPeExecutable(const QString &peExecutableFileName, QString *errorMessage,
                      QStringList *dependentLibrariesIn, unsigned *wordSizeIn,
                      bool *isDebugIn, bool isMinGW, unsigned short *machineArchIn)
{
    bool result = false;
    HANDLE hFile = NULL;
    HANDLE hFileMap = NULL;
    void *fileMemory = 0;

    if (dependentLibrariesIn)
        dependentLibrariesIn->clear();
    if (wordSizeIn)
        *wordSizeIn = 0;
    if (isDebugIn)
        *isDebugIn = false;

    do {
        // Create a memory mapping of the file
        hFile = CreateFile(reinterpret_cast<const WCHAR*>(peExecutableFileName.utf16()), GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) {
            *errorMessage = QString::fromLatin1("Cannot open '%1': %2")
                .arg(peExecutableFileName, QSystemError::windowsString());
            break;
        }

        hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (hFileMap == NULL) {
            *errorMessage = QString::fromLatin1("Cannot create file mapping of '%1': %2")
                .arg(peExecutableFileName, QSystemError::windowsString());
            break;
        }

        fileMemory = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
        if (!fileMemory) {
            *errorMessage = QString::fromLatin1("Cannot map '%1': %2")
                .arg(peExecutableFileName, QSystemError::windowsString());
            break;
        }

        const IMAGE_NT_HEADERS *ntHeaders = getNtHeader(fileMemory, errorMessage);
        if (!ntHeaders)
            break;

        const unsigned wordSize = ntHeaderWordSize(ntHeaders);
        if (wordSizeIn)
            *wordSizeIn = wordSize;
        if (wordSize == 32) {
            determineDebugAndDependentLibs(reinterpret_cast<const IMAGE_NT_HEADERS32 *>(ntHeaders),
                                           fileMemory, dependentLibrariesIn, isDebugIn, errorMessage);
        } else {
            determineDebugAndDependentLibs(reinterpret_cast<const IMAGE_NT_HEADERS64 *>(ntHeaders),
                                           fileMemory, dependentLibrariesIn, isDebugIn, errorMessage);
        }

        if (machineArchIn)
            *machineArchIn = ntHeaders->FileHeader.Machine;

        result = true;
        if (optVerboseLevel > 1) {
            std::wcout << __FUNCTION__ << ": " << QDir::toNativeSeparators(peExecutableFileName)
                << ' ' << wordSize << " bit";
            if (isMinGW)
                std::wcout << ", MinGW";
            if (dependentLibrariesIn) {
                std::wcout << ", dependent libraries: ";
                if (optVerboseLevel > 2)
                    std::wcout << dependentLibrariesIn->join(u' ');
                else
                    std::wcout << dependentLibrariesIn->size();
            }
            if (isDebugIn)
                std::wcout << (*isDebugIn ? ", debug" : ", release");
            std::wcout << '\n';
        }
    } while (false);

    if (fileMemory)
        UnmapViewOfFile(fileMemory);

    if (hFileMap != NULL)
        CloseHandle(hFileMap);

    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return result;
}

QString findD3dCompiler(Platform platform, const QString &qtBinDir, unsigned wordSize)
{
    const QString prefix = QStringLiteral("D3Dcompiler_");
    const QString suffix = QLatin1StringView(windowsSharedLibrarySuffix);
    // Get the DLL from Kit 8.0 onwards
    const QString kitDir = QString::fromLocal8Bit(qgetenv("WindowsSdkDir"));
    if (!kitDir.isEmpty()) {
        QString redistDirPath = QDir::cleanPath(kitDir) + QStringLiteral("/Redist/D3D/");
        if (platform.testFlag(ArmBased)) {
            redistDirPath += QStringLiteral("arm");
        } else {
            redistDirPath += wordSize == 32 ? QStringLiteral("x86") : QStringLiteral("x64");
        }
        QDir redistDir(redistDirPath);
        if (redistDir.exists()) {
            const QFileInfoList files = redistDir.entryInfoList(QStringList(prefix + u'*' + suffix), QDir::Files);
            if (!files.isEmpty())
                return files.front().absoluteFilePath();
        }
    }
    QStringList candidateVersions;
    for (int i = 47 ; i >= 40 ; --i)
        candidateVersions.append(prefix + QString::number(i) + suffix);
    // Check the bin directory of the Qt SDK (in case it is shadowed by the
    // Windows system directory in PATH).
    for (const QString &candidate : std::as_const(candidateVersions)) {
        const QFileInfo fi(qtBinDir + u'/' + candidate);
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    // Find the latest D3D compiler DLL in path (Windows 8.1 has d3dcompiler_47).
    if (platform.testFlag(IntelBased)) {
        QString errorMessage;
        unsigned detectedWordSize;
        for (const QString &candidate : std::as_const(candidateVersions)) {
            const QString dll = findInPath(candidate);
            if (!dll.isEmpty()
                && readPeExecutable(dll, &errorMessage, 0, &detectedWordSize, 0)
                && detectedWordSize == wordSize) {
                return dll;
            }
        }
    }
    return QString();
}

QStringList findDxc(Platform platform, const QString &qtBinDir, unsigned wordSize)
{
    QStringList results;
    const QString kitDir = QString::fromLocal8Bit(qgetenv("WindowsSdkDir"));
    const QString suffix = QLatin1StringView(windowsSharedLibrarySuffix);
    for (QString prefix : { QStringLiteral("dxcompiler"), QStringLiteral("dxil") }) {
        QString name = prefix + suffix;
        if (!kitDir.isEmpty()) {
            QString redistDirPath = QDir::cleanPath(kitDir) + QStringLiteral("/Redist/D3D/");
            if (platform.testFlag(ArmBased)) {
                redistDirPath += wordSize == 32 ? QStringLiteral("arm") : QStringLiteral("arm64");
            } else {
                redistDirPath += wordSize == 32 ? QStringLiteral("x86") : QStringLiteral("x64");
            }
            QDir redistDir(redistDirPath);
            if (redistDir.exists()) {
                const QFileInfoList files = redistDir.entryInfoList(QStringList(prefix + u'*' + suffix), QDir::Files);
                if (!files.isEmpty()) {
                    results.append(files.front().absoluteFilePath());
                    continue;
                }
            }
        }
        // Check the bin directory of the Qt SDK (in case it is shadowed by the
        // Windows system directory in PATH).
        const QFileInfo fi(qtBinDir + u'/' + name);
        if (fi.isFile()) {
            results.append(fi.absoluteFilePath());
            continue;
        }
        // Try to find it in the PATH (e.g. the Vulkan SDK ships these, even if Windows itself doesn't).
        if (platform.testFlag(IntelBased)) {
            QString errorMessage;
            unsigned detectedWordSize;
            const QString dll = findInPath(name);
            if (!dll.isEmpty()
                && readPeExecutable(dll, &errorMessage, 0, &detectedWordSize, 0)
                && detectedWordSize == wordSize)
            {
                results.append(dll);
                continue;
            }
        }
    }
    return results;
}

#else // Q_OS_WIN

bool readPeExecutable(const QString &, QString *errorMessage,
                      QStringList *, unsigned *, bool *, bool, unsigned short *)
{
    *errorMessage = QStringLiteral("Not implemented.");
    return false;
}

QString findD3dCompiler(Platform, const QString &, unsigned)
{
    return QString();
}

QStringList findDxc(Platform, const QString &, unsigned)
{
    return QStringList();
}

#endif // !Q_OS_WIN

// Search for "qt_prfxpath=xxxx" in \a path, and replace it with "qt_prfxpath=."
bool patchQtCore(const QString &path, QString *errorMessage)
{
    if (optVerboseLevel)
        std::wcout << "Patching " << QFileInfo(path).fileName() << "...\n";

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *errorMessage = QString::fromLatin1("Unable to patch %1: %2").arg(
                    QDir::toNativeSeparators(path), file.errorString());
        return false;
    }
    const QByteArray oldContent = file.readAll();

    if (oldContent.isEmpty()) {
        *errorMessage = QString::fromLatin1("Unable to patch %1: Could not read file content").arg(
                    QDir::toNativeSeparators(path));
        return false;
    }
    file.close();

    QByteArray content = oldContent;

    QByteArray prfxpath("qt_prfxpath=");
    int startPos = content.indexOf(prfxpath);
    if (startPos == -1) {
        *errorMessage = QString::fromLatin1(
                    "Unable to patch %1: Could not locate pattern \"qt_prfxpath=\"").arg(
                    QDir::toNativeSeparators(path));
        return false;
    }
    startPos += prfxpath.length();
    int endPos = content.indexOf(char(0), startPos);
    if (endPos == -1) {
        *errorMessage = QString::fromLatin1("Unable to patch %1: Internal error").arg(
                    QDir::toNativeSeparators(path));
        return false;
    }

    QByteArray replacement = QByteArray(endPos - startPos, char(0));
    replacement[0] = '.';
    content.replace(startPos, endPos - startPos, replacement);
    if (content == oldContent)
        return true;

    if (!file.open(QIODevice::WriteOnly)
        || (file.write(content) != content.size())) {
        *errorMessage = QString::fromLatin1("Unable to patch %1: Could not write to file: %2").arg(
                    QDir::toNativeSeparators(path), file.errorString());
        return false;
    }
    return true;
}

#ifdef Q_OS_WIN
QString getArchString(unsigned short machineArch)
{
    switch (machineArch) {
        case IMAGE_FILE_MACHINE_I386:
            return QStringLiteral("x86");
        case IMAGE_FILE_MACHINE_ARM:
            return QStringLiteral("arm");
        case IMAGE_FILE_MACHINE_AMD64:
            return QStringLiteral("x64");
        case IMAGE_FILE_MACHINE_ARM64:
            return QStringLiteral("arm64");
        default:
            break;
    }
    return QString();
}
#endif // Q_OS_WIN

QT_END_NAMESPACE
