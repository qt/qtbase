/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cesdkhandler.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qdebug.h>
#include <qxmlstream.h>
#include <qsettings.h>
#include <qtextstream.h>

QT_BEGIN_NAMESPACE

struct PropertyContainer
{
    void clear() { name.clear(); value.clear(); properties.clear(); }
    QString name;
    QString value;
    QMap<QString, PropertyContainer> properties;
};
Q_DECLARE_TYPEINFO(PropertyContainer, Q_MOVABLE_TYPE);

CeSdkInfo::CeSdkInfo() : m_major(0) , m_minor(0)
{
}

CeSdkHandler::CeSdkHandler()
{
}

struct ContainsPathKey
{
    bool operator()(const QString &val) const
    {
        return !(val.endsWith(QLatin1String("MSBuildToolsPath"))
                 || val.endsWith(QLatin1String("MSBuildToolsRoot")));
    }
};

struct ValueFromKey
{
    explicit ValueFromKey(const QSettings *settings) : settings(settings) {}
    QString operator()(const QString &key) const
    {
        return settings->value(key).toString();
    }

    const QSettings *settings;
};

bool CeSdkHandler::parseMsBuildFile(QFile *file, CeSdkInfo *info)
{
    bool result = file->open(QFile::ReadOnly | QFile::Text);
    const QString IncludePath = QStringLiteral("IncludePath");
    const QString LibraryPath = QStringLiteral("LibraryPath");
    const QString PreprocessorDefinitions = QStringLiteral("PreprocessorDefinitions");
    const QString SdkRootPathString = QStringLiteral("SdkRootPath");
    const QString ExecutablePath = QStringLiteral("ExecutablePath");
    enum ParserState{Not, Include, Lib, Define, BinDir, SdkRootPath};
    QString includePath;
    QString libraryPath;
    QString defines;
    QString binDirs;
    QString sdkRootPath;
    ParserState state = Not;
    if (result) {
        QXmlStreamReader xml(file);
        while (!xml.atEnd()) {
            if (xml.isStartElement()) {
                if (xml.name() == IncludePath)
                    state = Include;
                else if (xml.name() == LibraryPath)
                    state = Lib;
                else if (xml.name() == PreprocessorDefinitions)
                    state = Define;
                else if (xml.name() == SdkRootPathString)
                    state = SdkRootPath;
                else if (xml.name() == ExecutablePath)
                    state = BinDir;
                else
                    state = Not;
            } else if (xml.isEndElement()) {
                state = Not;
            } else if (xml.isCharacters()) {
                switch (state) {
                case Include:
                    includePath += xml.text();
                    break;
                case Lib:
                    libraryPath += xml.text();
                    break;
                case Define:
                    defines += xml.text();
                    break;
                case SdkRootPath:
                    sdkRootPath = xml.text().toString();
                    break;
                case BinDir:
                    binDirs += xml.text();
                case(Not):
                    break;
                }
            }
            xml.readNext();
        }
    }
    file->close();
    const bool success = result && !includePath.isEmpty() && !libraryPath.isEmpty() &&
            !defines.isEmpty() && !sdkRootPath.isEmpty();
    if (success) {
        const QString startPattern = QStringLiteral("$(Registry:");
        const int startIndex = sdkRootPath.indexOf(startPattern);
        const int endIndex = sdkRootPath.lastIndexOf(QLatin1Char(')'));
        const QString regString = sdkRootPath.mid(startIndex + startPattern.size(),
                                                  endIndex - startIndex - startPattern.size());
        QSettings sdkRootPathRegistry(regString, QSettings::NativeFormat);
        const QString erg = sdkRootPathRegistry.value(QStringLiteral(".")).toString();
        const QString fullSdkRootPath = erg + sdkRootPath.mid(endIndex + 1);
        const QLatin1String rootString("$(SdkRootPath)");

        includePath = includePath.replace(rootString, fullSdkRootPath);
        libraryPath = libraryPath.replace(rootString, fullSdkRootPath);
        binDirs = binDirs.replace(rootString, fullSdkRootPath);
        info->m_include = includePath + ";$(INCLUDE)";
        info->m_lib = libraryPath;
        info->m_bin = binDirs;
    }
    return success;
}

QStringList CeSdkHandler::getMsBuildToolPaths() const
{
    QSettings msbuildEntries("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\MSBuild\\ToolsVersions",
                             QSettings::NativeFormat);
    const QStringList allKeys = msbuildEntries.allKeys();
    QStringList toolVersionKeys;
    toolVersionKeys.push_back(QStringLiteral("c:\\Program Files\\MSBuild\\"));
    std::remove_copy_if(allKeys.cbegin(), allKeys.cend(),
                        std::back_inserter(toolVersionKeys), ContainsPathKey());
    QStringList toolVersionValues;
    std::transform(toolVersionKeys.constBegin(), toolVersionKeys.constEnd(),
                   std::back_inserter(toolVersionValues),
                   ValueFromKey(&msbuildEntries));
    return toolVersionValues;
}

QStringList CeSdkHandler::filterMsBuildToolPaths(const QStringList &paths) const
{
    QStringList result;
    for (const QString &path : paths) {
        QDir dirVC110(path);
        if (path.endsWith(QLatin1String("bin")))
            dirVC110.cdUp();
        QDir dirVC120 = dirVC110;
        if (dirVC110.cd(QStringLiteral("Microsoft.Cpp\\v4.0\\V110\\Platforms")))
            result << dirVC110.absolutePath();
        if (dirVC120.cd(QStringLiteral("Microsoft.Cpp\\v4.0\\V120\\Platforms")))
            result << dirVC120.absolutePath();
    }
    return result;
}

bool CeSdkHandler::retrieveEnvironment(const QStringList &relativePaths,
                                       const QStringList &toolPaths,
                                       CeSdkInfo *info)
{
    bool result = false;
    for (const QString &path : toolPaths) {
        const QDir dir(path);
        for (const QString &filePath : relativePaths) {
            QFile file(dir.absoluteFilePath(filePath));
            if (file.exists())
                result = parseMsBuildFile(&file, info) || result;
        }
    }

    return result;
}

void CeSdkHandler::retrieveWEC2013SDKs()
{
    const QStringList toolPaths = getMsBuildToolPaths();
    const QStringList filteredToolPaths = filterMsBuildToolPaths(toolPaths);
    QSettings settings("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows CE Tools\\SDKs", QSettings::NativeFormat);
    const QStringList keys = settings.allKeys();
    for (const QString &key : keys) {
        if (key.contains(QLatin1String("SDKInformation")) || key.contains(QLatin1Char('.'))) {
            QFile sdkPropertyFile(settings.value(key).toString());
            if (!sdkPropertyFile.exists())
                continue;
            QFileInfo info(sdkPropertyFile);
            if (info.isDir()) {
                const QDir dir = info.absoluteFilePath();
                QFileInfo fInfo(dir.filePath(QLatin1String("Properties.xml")));
                if (fInfo.exists())
                    sdkPropertyFile.setFileName(fInfo.absoluteFilePath());
            }
            if (!sdkPropertyFile.open(QFile::ReadOnly))
                continue;
            QXmlStreamReader xml(&sdkPropertyFile);
            QString currentElement;
            QString curName;
            PropertyContainer currentProperty;
            QVector<PropertyContainer> propStack;
            propStack.push_back(currentProperty);
            while (!xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement()) {
                    currentElement = xml.name().toString();
                    if (currentElement == QLatin1String("Property")) {
                        QXmlStreamAttributes attributes = xml.attributes();
                        if (attributes.hasAttribute(QLatin1String("NAME")))
                            curName = attributes.value(QLatin1String("NAME")).toString();
                        Q_ASSERT(!curName.isEmpty());
                        currentProperty.clear();
                        currentProperty.name = curName;
                        propStack.push_back(currentProperty);
                    } else if (currentElement == QLatin1String("PropertyBag")) {
                        QXmlStreamAttributes attributes = xml.attributes();
                        if (attributes.hasAttribute(QLatin1String("NAME")))
                            curName = attributes.value(QLatin1String("NAME")).toString();
                        Q_ASSERT(!curName.isEmpty());
                        currentProperty.clear();
                        currentProperty.name = curName;
                        propStack.push_back(currentProperty);
                    }
                } else if (xml.isEndElement()) {
                    currentElement = xml.name().toString();
                    PropertyContainer self = propStack.takeLast();
                    if (currentElement != QLatin1String("Root")) {
                        PropertyContainer &last = propStack.last();
                        last.properties[self.name] = self;
                    } else {
                        currentProperty = self;
                    }
                } else if (xml.isCharacters()) {
                    PropertyContainer &self = propStack.last();
                    self.value = xml.text().toString();
                }
            }

            if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
                qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
                return;
            }
            CeSdkInfo currentSdk;
            const PropertyContainer &cpuInfo = currentProperty.properties.value(QLatin1String("CPU info"));
            if (cpuInfo.properties.isEmpty())
                continue;
            const PropertyContainer &cpuInfoVal = cpuInfo.properties.first().properties.value(QLatin1String("CpuName"));
            if (cpuInfoVal.name != QLatin1String("CpuName"))
                continue;
            const QString SDKName = QStringLiteral("SDK name");
            currentSdk.m_name = currentProperty.properties.value(SDKName).value+
                                QStringLiteral(" (") + cpuInfoVal.value + ")";
            currentSdk.m_major = currentProperty.properties.value(QLatin1String("OSMajor")).value.toInt();
            currentSdk.m_minor = currentProperty.properties.value(QLatin1String("OSMinor")).value.toInt();
            retrieveEnvironment(currentProperty.properties.value(QLatin1String("MSBuild Files110")).value.split(';'),
                                filteredToolPaths, &currentSdk);
            retrieveEnvironment(currentProperty.properties.value(QLatin1String("MSBuild Files120")).value.split(';'),
                                filteredToolPaths, &currentSdk);
            if (!currentSdk.m_include.isEmpty())
                m_list.append(currentSdk);
        }
    }
}

void CeSdkHandler::retrieveWEC6n7SDKs()
{
    // look at the file at %VCInstallDir%/vcpackages/WCE.VCPlatform.config
    // and scan through all installed sdks...
    m_vcInstallDir = QString::fromLatin1(qgetenv("VCInstallDir"));
    if (m_vcInstallDir.isEmpty())
        return;

    QDir vStudioDir(m_vcInstallDir);
    if (!vStudioDir.cd(QLatin1String("vcpackages")))
        return;

    QFile configFile(vStudioDir.absoluteFilePath(QLatin1String("WCE.VCPlatform.config")));
    if (!configFile.open(QIODevice::ReadOnly))
        return;

    QString currentElement;
    CeSdkInfo currentItem;
    QXmlStreamReader xml(&configFile);
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            currentElement = xml.name().toString();
            if (currentElement == QLatin1String("Platform")) {
                currentItem = CeSdkInfo();
            } else if (currentElement == QLatin1String("Directories")) {
                QXmlStreamAttributes attr = xml.attributes();
                currentItem.m_include = fixPaths(attr.value(QLatin1String("Include")).toString());
                currentItem.m_lib = fixPaths(attr.value(QLatin1String("Library")).toString());
                currentItem.m_bin = fixPaths(attr.value(QLatin1String("Path")).toString());
            }
        } else if (xml.isEndElement()) {
            if (xml.name().toString() == QLatin1String("Platform"))
                m_list.append(currentItem);
        } else if (xml.isCharacters() && !xml.isWhitespace()) {
            if (currentElement == QLatin1String("PlatformName"))
                currentItem.m_name = xml.text().toString();
            else if (currentElement == QLatin1String("OSMajorVersion"))
                currentItem.m_major = xml.text().toString().toInt();
            else if (currentElement == QLatin1String("OSMinorVersion"))
                currentItem.m_minor = xml.text().toString().toInt();
        }
    }

    if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
        return;
    }
}

bool CeSdkHandler::retrieveAvailableSDKs()
{
    m_list.clear();
    retrieveWEC2013SDKs();
    retrieveWEC6n7SDKs();
    return !m_list.empty();
}

QString CeSdkHandler::fixPaths(const QString &path) const
{
    QRegExp searchStr(QLatin1String("(\\$\\(\\w+\\))"));
    QString fixedString = path;
    for (int index = fixedString.indexOf(searchStr, 0);
         index >= 0;
         index = fixedString.indexOf(searchStr, index)) {
        const QString capture = searchStr.cap(0);
        fixedString.replace(index, capture.length(), capture.toUpper());
        index += capture.length(); // don't count the zero terminator
        fixedString.insert(index, '\\'); // the configuration file lacks a directory separator for env vars
        ++index;
    }
    return fixedString;
}

QT_END_NAMESPACE
