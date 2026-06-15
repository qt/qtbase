// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "common.h"
#include "jsontools.h"
#include "wasmbinary.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirListing>
#include <QDirIterator>
#include <QtGlobal>
#include <QLibraryInfo>
#include <QJsonDocument>
#include <QStringList>
#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QProcess>
#include <QQueue>
#include <QMap>
#include <QSet>

#include <optional>
#include <iostream>
#include <ostream>

struct Parameters
{
    std::optional<QString> argAppPath;
    QString appWasmPath;
    std::optional<QDir> qtHostDir;
    std::optional<QDir> qtWasmDir;
    QList<QDir> libPaths;
    std::optional<QDir> qmlRootPath;

    QSet<QString> loadedQtLibraries;
};

bool parseArguments(Parameters &params)
{
    QCoreApplication::setApplicationName("wasmdeployqt");
    QCoreApplication::setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription(
            QStringLiteral("Qt for WebAssembly deployment tool \n\n"
                           "Example:\n"
                           "wasmdeployqt app.wasm --qml-root-path=repo/myapp "
                           "--qt-wasm-dir=/home/user/qt/shared-qt-wasm/bin"));
    parser.addHelpOption();

    QStringList args = QCoreApplication::arguments();

    parser.addPositionalArgument("app", "Path to the application.");
    QCommandLineOption libPathOption("lib-path", "Colon-separated list of library directories.",
                                     "paths");
    parser.addOption(libPathOption);
    QCommandLineOption qtWasmDirOption("qt-wasm-dir", "Path to the Qt for WebAssembly directory.",
                                       "dir");
    parser.addOption(qtWasmDirOption);
    QCommandLineOption qtHostDirOption("qt-host-dir", "Path to the Qt host directory.", "dir");
    parser.addOption(qtHostDirOption);
    QCommandLineOption qmlRootPathOption("qml-root-path", "Root directory for QML files.", "dir");
    parser.addOption(qmlRootPathOption);
    parser.process(args);

    const QStringList positionalArgs = parser.positionalArguments();
    if (positionalArgs.size() > 1) {
        std::cout << "ERROR: Expected only one positional argument with path to the app. Received: "
                  << positionalArgs.join(" ").toStdString() << std::endl;
        return false;
    }
    if (!positionalArgs.isEmpty()) {
        params.argAppPath = positionalArgs.first();
    }

    if (parser.isSet(libPathOption)) {
        QStringList paths = parser.value(libPathOption).split(';', Qt::SkipEmptyParts);
        for (const QString &path : paths) {
            QDir dir(path);
            if (dir.exists()) {
                params.libPaths.append(dir);
            } else {
                std::cout << "ERROR: Directory does not exist: " << path.toStdString() << std::endl;
                return false;
            }
        }
    }
    if (parser.isSet(qtWasmDirOption)) {
        QDir dir(parser.value(qtWasmDirOption));
        if (dir.cdUp() && dir.exists())
            params.qtWasmDir = dir;
        else {
            std::cout << "ERROR: Directory does not exist: " << dir.absolutePath().toStdString()
                      << std::endl;
            return false;
        }
    }
    if (parser.isSet(qtHostDirOption)) {
        QDir dir(parser.value(qtHostDirOption));
        if (dir.cdUp() && dir.exists())
            params.qtHostDir = dir;
        else {
            std::cout << "ERROR: Directory does not exist: " << dir.absolutePath().toStdString()
                      << std::endl;
            return false;
        }
    }
    if (parser.isSet(qmlRootPathOption)) {
        QDir dir(parser.value(qmlRootPathOption));
        if (dir.exists()) {
            params.qmlRootPath = dir;
        } else {
            std::cout << "ERROR: Directory specified for qml-root-path does not exist: "
                      << dir.absolutePath().toStdString() << std::endl;
            return false;
        }
    }
    return true;
}

std::optional<QString> detectAppName()
{
    QDirIterator it(QDir::currentPath(), QStringList() << "*.html" << "*.wasm" << "*.js",
                    QDir::NoFilter);
    QMap<QString, QSet<QString>> fileGroups;
    while (it.hasNext()) {
        QFileInfo fileInfo(it.next());
        QString baseName = fileInfo.completeBaseName();
        QString suffix = fileInfo.suffix();
        fileGroups[baseName].insert(suffix);
    }
    for (auto it = fileGroups.constBegin(); it != fileGroups.constEnd(); ++it) {
        const QSet<QString> &extensions = it.value();
        if (extensions.contains("html") && extensions.contains("js")
            && extensions.contains("wasm")) {
            return it.key();
        }
    }
    return std::nullopt;
}

bool verifyPaths(Parameters &params)
{
    if (params.argAppPath) {
        QFileInfo fileInfo(*params.argAppPath);
        if (!fileInfo.exists()) {
            std::cout << "ERROR: Cannot find " << params.argAppPath->toStdString() << std::endl;
            std::cout << "Make sure that the path is valid." << std::endl;
            return false;
        }
        params.appWasmPath = fileInfo.absoluteFilePath();
    } else {
        auto appName = detectAppName();
        if (!appName) {
            std::cout << "ERROR: Cannot find the application in current directory. Specify the "
                         "path as an argument:"
                         "wasmdeployqt <path-to-app-wasm-binary>"
                      << std::endl;
            return false;
        }
        params.appWasmPath = QDir::current().filePath(*appName + ".wasm");
        std::cout << "Automatically detected " << params.appWasmPath.toStdString() << std::endl;
    }
    if (!params.qtWasmDir) {
        std::cout << "ERROR: Please set path to Qt WebAssembly installation as "
                     "--qt-wasm-dir=<path_to_qt_wasm_bin>"
                  << std::endl;
        return false;
    }
    if (!params.qtHostDir) {
        QStringList qtHostPaths = QLibraryInfo::paths(QLibraryInfo::BinariesPath);
        if (qtHostPaths.isEmpty()) {
            std::cout << "ERROR: Cannot read Qt host path or detect it from environment. Please "
                         "pass it explicitly with --qt-host-dir=<path>. "
                      << std::endl;
        } else {
            auto qtHostDir = QDir(qtHostPaths.at(0));
            if (!qtHostDir.cdUp()) {
                std::cout << "ERROR: Invalid Qt host path: "
                          << qtHostDir.absolutePath().toStdString() << std::endl;
                return false;
            }
            params.qtHostDir = qtHostDir;
        }
    }
    params.libPaths.push_front(params.qtWasmDir->filePath("lib"));
    params.libPaths.push_front(*params.qtWasmDir);
    return true;
}

bool copyFile(QString srcPath, QString destPath)
{
    auto file = QFile(destPath);
    if (file.exists()) {
        file.remove();
    }
    QFileInfo destInfo(destPath);
    if (!QDir().mkpath(destInfo.path())) {
        std::cout << "ERROR: Cannot create path " << destInfo.path().toStdString() << std::endl;
        return false;
    }
    if (!QFile::copy(srcPath, destPath)) {

        std::cout << "ERROR: Failed to copy " << srcPath.toStdString() << " to "
                  << destPath.toStdString() << std::endl;

        return false;
    }
    return true;
}

bool copyDirectDependencies(QList<QString> dependencies, const Parameters &params)
{
    for (auto &&depFilename : dependencies) {
        if (params.loadedQtLibraries.contains(depFilename)) {
            continue; // dont copy library that has been already copied
        }

        std::optional<QString> libPath;
        for (auto &&libDir : params.libPaths) {
            auto path = libDir.filePath(depFilename);
            QFileInfo file(path);
            if (file.exists()) {
                libPath = path;
            }
        }
        if (!libPath) {
            std::cout << "ERROR: Cannot find required library " << depFilename.toStdString()
                      << std::endl;
            return false;
        }
        if (!copyFile(*libPath, QDir::current().filePath(depFilename)))
            return false;
    }
    std::cout << "INFO: Succesfully copied direct dependencies." << std::endl;
    return true;
}

QStringList findSoFiles(const QString &directory)
{
    QStringList soFiles;
    QDir baseDir(directory);
    if (!baseDir.exists())
        return soFiles;

    QDirIterator it(directory, QStringList() << "*.so", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString absPath = it.filePath();
        QString filePath = baseDir.relativeFilePath(absPath);
        soFiles.append(filePath);
    }
    return soFiles;
}

bool copyQtLibs(Parameters &params)
{
    Q_ASSERT(params.qtWasmDir);
    auto qtLibDir = *params.qtWasmDir;
    if (!qtLibDir.cd("lib")) {
        std::cout << "ERROR: Cannot find lib directory in Qt installation." << std::endl;
        return false;
    }
    auto qtLibTargetDir = QDir(QDir(QDir::current().filePath("qt")).filePath("lib"));

    auto soFiles = findSoFiles(qtLibDir.absolutePath());
    for (auto &&soFilePath : soFiles) {
        auto relativeFilePath = QDir("lib").filePath(soFilePath);
        auto srcPath = qtLibDir.absoluteFilePath(soFilePath);
        auto destPath = qtLibTargetDir.absoluteFilePath(soFilePath);
        if (!copyFile(srcPath, destPath))
            return false;
        params.loadedQtLibraries.insert(QFileInfo(srcPath).fileName());
    }
    std::cout << "INFO: Succesfully deployed qt lib shared objects." << std::endl;
    return true;
}

bool copyPreloadPlugins(Parameters &params)
{
    Q_ASSERT(params.qtWasmDir);
    auto qtPluginsDir = *params.qtWasmDir;
    if (!qtPluginsDir.cd("plugins")) {
        std::cout << "ERROR: Cannot find plugins directory in Qt installation." << std::endl;
        return false;
    }
    auto qtPluginsTargetDir = QDir(QDir(QDir::current().filePath("qt")).filePath("plugins"));

    // copy files
    auto soFiles = findSoFiles(qtPluginsDir.absolutePath());
    for (auto &&soFilePath : soFiles) {
        auto relativeFilePath = QDir("plugins").filePath(soFilePath);
        params.loadedQtLibraries.insert(QFileInfo(relativeFilePath).fileName());
        auto srcPath = qtPluginsDir.absoluteFilePath(soFilePath);
        auto destPath = qtPluginsTargetDir.absoluteFilePath(soFilePath);
        if (!copyFile(srcPath, destPath))
            return false;
    }

    // qt_plugins.json
    QSet<PreloadEntry> preload{ { { "qt.conf" }, { "/qt.conf" } } };
    for (auto &&plugin : soFiles) {
        PreloadEntry entry;
        entry.source = QDir("$QTDIR").filePath("plugins") + QDir::separator()
                + QDir(qtPluginsDir).relativeFilePath(plugin);
        entry.destination = "/qt/plugins/" + QDir(qtPluginsTargetDir).relativeFilePath(plugin);
        preload.insert(entry);
    }
    JsonTools::savePreloadFile(preload, QDir::current().filePath("qt_plugins.json"));

    QString qtconfContent = "[Paths]\nPrefix = /qt\n";
    QString filePath = QDir::current().filePath("qt.conf");

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << qtconfContent;
        if (!file.flush()) {
            std::cout << "ERROR: Failed flushing the file :" << file.fileName().toStdString()
                      << std::endl;
            return false;
        }
        file.close();
    } else {
        std::cout << "ERROR: Failed to write to qt.conf." << std::endl;
        return false;
    }
    std::cout << "INFO: Succesfully deployed qt plugins." << std::endl;
    return true;
}

bool copyPreloadQmlImports(Parameters &params)
{
    Q_ASSERT(params.qtWasmDir);
    if (!params.qmlRootPath) {
        std::cout << "WARNING: qml-root-path not specified. Skipping generating preloads for QML "
                     "imports."
                  << std::endl;
        std::cout << "WARNING: This may lead to erronous behaviour if applications requires QML "
                     "imports."
                  << std::endl;
        QSet<PreloadEntry> preload;
        JsonTools::savePreloadFile(preload, QDir::current().filePath("qt_qml_imports.json"));
        return true;
    }
    auto qmlImportScannerPath = params.qtHostDir
            ? QDir(params.qtHostDir->filePath("libexec")).filePath("qmlimportscanner")
            : "qmlimportscanner";
    QProcess process;
    auto qmlImportPath = *params.qtWasmDir;
    qmlImportPath.cd("qml");
    if (!qmlImportPath.exists()) {
        std::cout << "ERROR: Cannot find qml import path: "
                  << qmlImportPath.absolutePath().toStdString() << std::endl;
        return false;
    }

    QStringList args{ "-rootPath", params.qmlRootPath->absolutePath(), "-importPath",
                      qmlImportPath.absolutePath() };
    process.start(qmlImportScannerPath, args);
    if (!process.waitForFinished()) {
        std::cout << "ERROR: Failed to execute qmlImportScanner." << std::endl;
        return false;
    }

    QString stdoutOutput = process.readAllStandardOutput();
    auto qmlImports = JsonTools::getPreloadsFromQmlImportScannerOutput(stdoutOutput);
    if (!qmlImports) {
        return false;
    }
    JsonTools::savePreloadFile(*qmlImports, QDir::current().filePath("qt_qml_imports.json"));
    for (const PreloadEntry &import : *qmlImports) {
        auto relativePath = import.source;
        relativePath.remove("$QTDIR/");

        auto srcPath = params.qtWasmDir->absoluteFilePath(relativePath);
        auto destPath = QDir(QDir::current().filePath("qt")).absoluteFilePath(relativePath);
        if (!copyFile(srcPath, destPath))
            return false;
    }
    std::cout << "INFO: Succesfully deployed qml imports." << std::endl;
    return true;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Parameters params;
    if (!parseArguments(params)) {
        return -1;
    }
    if (!verifyPaths(params)) {
        return -1;
    }
    std::cout << "INFO: Target: " << params.appWasmPath.toStdString() << std::endl;
    WasmBinary wasmBinary(params.appWasmPath);
    if (wasmBinary.type == WasmBinary::Type::INVALID) {
        return -1;
    } else if (wasmBinary.type == WasmBinary::Type::STATIC) {
        std::cout << "INFO: This is statically linked WebAssembly binary." << std::endl;
        std::cout << "INFO: No extra steps required!" << std::endl;
        return 0;
    }
    std::cout << "INFO: Verified as shared module." << std::endl;

    if (!copyQtLibs(params))
        return -1;
    if (!copyPreloadPlugins(params))
        return -1;
    if (!copyPreloadQmlImports(params))
        return -1;
    if (!copyDirectDependencies(wasmBinary.dependencies, params))
        return -1;

    std::cout << "INFO: Deployment done!" << std::endl;
    return 0;
}
