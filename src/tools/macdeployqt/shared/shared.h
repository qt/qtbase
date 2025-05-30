// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef MAC_DEPLOMYMENT_SHARED_H
#define MAC_DEPLOMYMENT_SHARED_H

#include <QString>
#include <QStringList>
#include <QDebug>
#include <QSet>
#include <QVersionNumber>

extern int logLevel;
#define LogError()      if (logLevel < 0) {} else qDebug() << "ERROR:"
#define LogWarning()    if (logLevel < 1) {} else qDebug() << "WARNING:"
#define LogNormal()     if (logLevel < 2) {} else qDebug() << "Log:"
#define LogDebug()      if (logLevel < 3) {} else qDebug() << "Log:"

extern bool runStripEnabled;

class FrameworkInfo
{
public:
    bool isDylib;
    QString frameworkDirectory;
    QString frameworkName;
    QString frameworkPath;
    QString binaryDirectory;
    QString binaryName;
    QString binaryPath;
    QString rpathUsed;
    QString version;
    QString installName;
    QString deployedInstallName;
    QString sourceFilePath;
    QString frameworkDestinationDirectory;
    QString binaryDestinationDirectory;

    bool isDebugLibrary() const
    {
        if (isDylib)
            return binaryName.contains(QStringLiteral("_debug."));
        else
            return binaryName.endsWith(QStringLiteral("_debug"));
    }
};

class DylibInfo
{
public:
    QString binaryPath;
    QVersionNumber currentVersion;
    QVersionNumber compatibilityVersion;
};

class OtoolInfo
{
public:
    QString installName;
    QString binaryPath;
    QVersionNumber currentVersion;
    QVersionNumber compatibilityVersion;
    QList<DylibInfo> dependencies;
};

bool operator==(const FrameworkInfo &a, const FrameworkInfo &b);
QDebug operator<<(QDebug debug, const FrameworkInfo &info);

class ApplicationBundleInfo
{
    public:
    QString path;
    QString binaryPath;
    QStringList libraryPaths;
};

class DeploymentInfo
{
public:
    QString qtPath;
    QString pluginPath;
    QStringList deployedFrameworks;
    QList<QString> rpathsUsed;
    bool useLoaderPath;
    bool isFramework;
    bool isDebug;

    bool containsModule(const QString &module, const QString &libInFix) const;
};

inline QDebug operator<<(QDebug debug, const ApplicationBundleInfo &info);

OtoolInfo findDependencyInfo(const QString &binaryPath);
FrameworkInfo parseOtoolLibraryLine(const QString &line, const QString &appBundlePath, const QList<QString> &rpaths, bool useDebugLibs);
QString findAppBinary(const QString &appBundlePath);
QList<FrameworkInfo> getQtFrameworks(const QString &path, const QString &appBundlePath, const QList<QString> &rpaths, bool useDebugLibs);
QList<FrameworkInfo> getQtFrameworks(const QStringList &otoolLines, const QString &appBundlePath, const QList<QString> &rpaths, bool useDebugLibs);
QString copyFramework(const FrameworkInfo &framework, const QString path);
DeploymentInfo deployQtFrameworks(const QString &appBundlePath, const QStringList &additionalExecutables, bool useDebugLibs);
DeploymentInfo deployQtFrameworks(QList<FrameworkInfo> frameworks,const QString &bundlePath, const QStringList &binaryPaths, bool useDebugLibs, bool useLoaderPath);
void createQtConf(const QString &appBundlePath);
void deployPlugins(const QString &appBundlePath, DeploymentInfo deploymentInfo, bool useDebugLibs);
bool deployQmlImports(const QString &appBundlePath, DeploymentInfo deploymentInfo, QStringList &qmlDirs, QStringList &qmlImportPaths);
void changeIdentification(const QString &id, const QString &binaryPath);
void changeInstallName(const QString &oldName, const QString &newName, const QString &binaryPath);
void runStrip(const QString &binaryPath);
void stripAppBinary(const QString &bundlePath);
QString findAppBinary(const QString &appBundlePath);
QStringList findAppFrameworkNames(const QString &appBundlePath);
QStringList findAppFrameworkPaths(const QString &appBundlePath);
void codesignFile(const QString &identity, const QString &filePath);
QSet<QString> codesignBundle(const QString &identity,
                             const QString &appBundlePath,
                             QList<QString> additionalBinariesContainingRpaths);
void codesign(const QString &identity, const QString &appBundlePath);
void createDiskImage(const QString &appBundlePath, const QString &filesystemType);
void fixupFramework(const QString &appBundlePath);


#endif
