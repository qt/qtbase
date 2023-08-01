// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef UTILS_H
#define UTILS_H

#include <QStringList>
#include <QMap>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>

#include <iostream>

QT_BEGIN_NAMESPACE

enum PlatformFlag {
    // OS
    WindowsBased = 0x00001,
    // CPU
    IntelBased   = 0x00010,
    ArmBased     = 0x00020,
    // Compiler
    Msvc         = 0x00100,
    MinGW        = 0x00200,
    ClangMsvc    = 0x00400,
    ClangMinGW   = 0x00800,
    // Platforms
    WindowsDesktopMsvc = WindowsBased + IntelBased + Msvc,
    WindowsDesktopMinGW = WindowsBased + IntelBased + MinGW,
    WindowsDesktopClangMsvc = WindowsBased + IntelBased + ClangMsvc,
    WindowsDesktopClangMinGW = WindowsBased + IntelBased + ClangMinGW,
    UnknownPlatform
};

Q_DECLARE_FLAGS(Platform, PlatformFlag)

Q_DECLARE_OPERATORS_FOR_FLAGS(Platform)

inline bool platformHasDebugSuffix(Platform p) // Uses 'd' debug suffix
{
    return p.testFlag(Msvc) || p.testFlag(ClangMsvc);
}

enum ListOption {
    ListNone = 0,
    ListSource,
    ListTarget,
    ListRelative,
    ListMapping
};

inline std::wostream &operator<<(std::wostream &str, const QString &s)
{
#ifdef Q_OS_WIN
    str << reinterpret_cast<const wchar_t *>(s.utf16());
#else
    str << s.toStdWString();
#endif
    return str;
}

// Container class for JSON output
class JsonOutput
{
    using SourceTargetMapping = QPair<QString, QString>;
    using SourceTargetMappings = QList<SourceTargetMapping>;

public:
    void addFile(const QString &source, const QString &target)
    {
        m_files.append(SourceTargetMapping(source, target));
    }

    void removeTargetDirectory(const QString &targetDirectory)
    {
        for (int i = m_files.size() - 1; i >= 0; --i) {
            if (m_files.at(i).second == targetDirectory)
                m_files.removeAt(i);
        }
    }

    QByteArray toJson() const
    {
        QJsonObject document;
        QJsonArray files;
        for (const SourceTargetMapping &mapping : m_files) {
            QJsonObject object;
            object.insert(QStringLiteral("source"), QDir::toNativeSeparators(mapping.first));
            object.insert(QStringLiteral("target"), QDir::toNativeSeparators(mapping.second));
            files.append(object);
        }
        document.insert(QStringLiteral("files"), files);
        return QJsonDocument(document).toJson();
    }
    QByteArray toList(ListOption option, const QDir &base) const
    {
        QByteArray list;
        for (const SourceTargetMapping &mapping : m_files) {
            const QString source = QDir::toNativeSeparators(mapping.first);
            const QString fileName = QFileInfo(mapping.first).fileName();
            const QString target = QDir::toNativeSeparators(mapping.second) + QDir::separator() + fileName;
            switch (option) {
            case ListNone:
                break;
            case ListSource:
                list += source.toUtf8() + '\n';
                break;
            case ListTarget:
                list += target.toUtf8() + '\n';
                break;
            case ListRelative:
                list += QDir::toNativeSeparators(base.relativeFilePath(target)).toUtf8() + '\n';
                break;
            case ListMapping:
                list += '"' + source.toUtf8() + "\" \"" + QDir::toNativeSeparators(base.relativeFilePath(target)).toUtf8() + "\"\n";
                break;
            }
        }
        return list;
    }
private:
    SourceTargetMappings m_files;
};

#ifdef Q_OS_WIN
QString normalizeFileName(const QString &name);
QString winErrorMessage(unsigned long error);
QString findSdkTool(const QString &tool);
#else // !Q_OS_WIN
inline QString normalizeFileName(const QString &name) { return name; }
#endif // !Q_OS_WIN

static const char windowsSharedLibrarySuffix[] = ".dll";

inline QString sharedLibrarySuffix() { return QLatin1StringView(windowsSharedLibrarySuffix); }
bool isBuildDirectory(Platform platform, const QString &dirName);

bool createSymbolicLink(const QFileInfo &source, const QString &target, QString *errorMessage);
bool createDirectory(const QString &directory, QString *errorMessage, bool dryRun);
QString findInPath(const QString &file);

extern const char *qmakeInfixKey; // Fake key containing the libinfix

QMap<QString, QString> queryQtPaths(const QString &qmakeBinary, QString *errorMessage);

enum DebugMatchMode {
    MatchDebug,
    MatchRelease,
    MatchDebugOrRelease
};

QStringList findSharedLibraries(const QDir &directory, Platform platform,
                                DebugMatchMode debugMatchMode,
                                const QString &prefix = QString());

bool updateFile(const QString &sourceFileName, const QStringList &nameFilters,
                const QString &targetDirectory, unsigned flags, JsonOutput *json, QString *errorMessage);
bool runProcess(const QString &binary, const QStringList &args,
                const QString &workingDirectory = QString(),
                unsigned long *exitCode = 0, QByteArray *stdOut = 0, QByteArray *stdErr = 0,
                QString *errorMessage = 0);

bool readPeExecutable(const QString &peExecutableFileName, QString *errorMessage,
                      QStringList *dependentLibraries = 0, unsigned *wordSize = 0,
                      bool *isDebug = 0, bool isMinGW = false, unsigned short *machineArch = nullptr);

#ifdef Q_OS_WIN
#  if !defined(IMAGE_FILE_MACHINE_ARM64)
#    define IMAGE_FILE_MACHINE_ARM64 0xAA64
#  endif
QString getArchString (unsigned short machineArch);
#endif // Q_OS_WIN

// Return dependent modules of executable files.

inline QStringList findDependentLibraries(const QString &executableFileName, QString *errorMessage)
{
    QStringList result;
    readPeExecutable(executableFileName, errorMessage, &result);
    return result;
}

QString findD3dCompiler(Platform platform, const QString &qtBinDir, unsigned wordSize);

bool patchQtCore(const QString &path, QString *errorMessage);

extern int optVerboseLevel;

// Recursively update a file or directory, matching DirectoryFileEntryFunction against the QDir
// to obtain the files.
enum UpdateFileFlag  {
    ForceUpdateFile = 0x1,
    SkipUpdateFile = 0x2,
    RemoveEmptyQmlDirectories = 0x4,
    SkipQmlDesignerSpecificsDirectories = 0x8
};

template <class DirectoryFileEntryFunction>
bool updateFile(const QString &sourceFileName,
                DirectoryFileEntryFunction directoryFileEntryFunction,
                const QString &targetDirectory,
                unsigned flags,
                JsonOutput *json,
                QString *errorMessage)
{
    const QFileInfo sourceFileInfo(sourceFileName);
    const QString targetFileName = targetDirectory + u'/' + sourceFileInfo.fileName();
    if (optVerboseLevel > 1)
        std::wcout << "Checking " << sourceFileName << ", " << targetFileName << '\n';

    if (!sourceFileInfo.exists()) {
        *errorMessage = QString::fromLatin1("%1 does not exist.").arg(QDir::toNativeSeparators(sourceFileName));
        return false;
    }

    const QFileInfo targetFileInfo(targetFileName);

    if (sourceFileInfo.isSymLink()) {
        const QString sourcePath = sourceFileInfo.symLinkTarget();
        const QString relativeSource = QDir(sourceFileInfo.absolutePath()).relativeFilePath(sourcePath);
        if (relativeSource.contains(u'/')) {
            *errorMessage = QString::fromLatin1("Symbolic links across directories are not supported (%1).")
                            .arg(QDir::toNativeSeparators(sourceFileName));
            return false;
        }

        // Update the linked-to file
        if (!updateFile(sourcePath, directoryFileEntryFunction, targetDirectory, flags, json, errorMessage))
            return false;

        if (targetFileInfo.exists()) {
            if (!targetFileInfo.isSymLink()) {
                *errorMessage = QString::fromLatin1("%1 already exists and is not a symbolic link.")
                                .arg(QDir::toNativeSeparators(targetFileName));
                return false;
            } // Not a symlink
            const QString relativeTarget = QDir(targetFileInfo.absolutePath()).relativeFilePath(targetFileInfo.symLinkTarget());
            if (relativeSource == relativeTarget) // Exists and points to same entry: happy.
                return true;
            QFile existingTargetFile(targetFileName);
            if (!(flags & SkipUpdateFile) && !existingTargetFile.remove()) {
                *errorMessage = QString::fromLatin1("Cannot remove existing symbolic link %1: %2")
                                .arg(QDir::toNativeSeparators(targetFileName), existingTargetFile.errorString());
                return false;
            }
        } // target symbolic link exists
        return createSymbolicLink(QFileInfo(targetDirectory + u'/' + relativeSource), sourceFileInfo.fileName(), errorMessage);
    } // Source is symbolic link

    if (sourceFileInfo.isDir()) {
        if ((flags & SkipQmlDesignerSpecificsDirectories) && sourceFileInfo.fileName() == QLatin1StringView("designer")) {
            if (optVerboseLevel)
                std::wcout << "Skipping " << QDir::toNativeSeparators(sourceFileName) << ".\n";
            return true;
        }
        bool created = false;
        if (targetFileInfo.exists()) {
            if (!targetFileInfo.isDir()) {
                *errorMessage = QString::fromLatin1("%1 already exists and is not a directory.")
                                .arg(QDir::toNativeSeparators(targetFileName));
                return false;
            } // Not a directory.
        } else { // exists.
            QDir d(targetDirectory);
            if (optVerboseLevel)
                std::wcout << "Creating " << targetFileName << ".\n";
            if (!(flags & SkipUpdateFile)) {
                created = d.mkdir(sourceFileInfo.fileName());
                if (!created) {
                    *errorMessage = QString::fromLatin1("Cannot create directory %1 under %2.")
                            .arg(sourceFileInfo.fileName(), QDir::toNativeSeparators(targetDirectory));
                    return false;
                }
            }
        }
        // Recurse into directory
        QDir dir(sourceFileName);

        const QStringList allEntries = directoryFileEntryFunction(dir) + dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &entry : allEntries)
            if (!updateFile(sourceFileName + u'/' + entry, directoryFileEntryFunction, targetFileName, flags, json, errorMessage))
                return false;
        // Remove empty directories, for example QML import folders for which the filter did not match.
        if (created && (flags & RemoveEmptyQmlDirectories)) {
            QDir d(targetFileName);
            const QStringList entries = d.entryList(QStringList(), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            if (entries.isEmpty() || (entries.size() == 1 && entries.first() == QLatin1StringView("qmldir"))) {
                if (!d.removeRecursively()) {
                    *errorMessage = QString::fromLatin1("Cannot remove empty directory %1.")
                            .arg(QDir::toNativeSeparators(targetFileName));
                    return false;
                }
                if (json)
                    json->removeTargetDirectory(targetFileName);
            }
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
    if (!(flags & SkipUpdateFile)) {
        if (!file.copy(targetFileName)) {
            *errorMessage = QString::fromLatin1("Cannot copy %1 to %2: %3")
                .arg(QDir::toNativeSeparators(sourceFileName),
                     QDir::toNativeSeparators(targetFileName),
                     file.errorString());
            return false;
        }
        if (!(file.permissions() & QFile::WriteUser)) { // QTBUG-40152, clear inherited read-only attribute
            QFile targetFile(targetFileName);
            if (!targetFile.setPermissions(targetFile.permissions() | QFile::WriteUser)) {
                *errorMessage = QString::fromLatin1("Cannot set write permission on %1: %2")
                    .arg(QDir::toNativeSeparators(targetFileName), file.errorString());
                return false;
            }
        } // Check permissions
    } // !SkipUpdateFile
    if (json)
        json->addFile(sourceFileName, targetDirectory);
    return true;
}

// Base class to filter files by name filters functions to be passed to updateFile().
class NameFilterFileEntryFunction {
public:
    explicit NameFilterFileEntryFunction(const QStringList &nameFilters) : m_nameFilters(nameFilters) {}
    QStringList operator()(const QDir &dir) const { return dir.entryList(m_nameFilters, QDir::Files); }

private:
    const QStringList m_nameFilters;
};

// Convenience for all files.
inline bool updateFile(const QString &sourceFileName, const QString &targetDirectory, unsigned flags, JsonOutput *json, QString *errorMessage)
{
    return updateFile(sourceFileName, NameFilterFileEntryFunction(QStringList()), targetDirectory, flags, json, errorMessage);
}

QT_END_NAMESPACE

#endif // UTILS_H
