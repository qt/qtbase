// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef TESTCOMPILER_H
#define TESTCOMPILER_H

#include <QObject>
#include <QStringList>

enum BuildType { Exe, Dll, Lib, Plain };

class TestCompiler : public QObject
{
    Q_OBJECT

public:
    TestCompiler();
    virtual ~TestCompiler();

    void setBaseCommands( QString makeCmd, QString qmakeCmd );

    void resetArguments();
    void setArguments(const QStringList &makeArgs, const QStringList &qmakeArgs);

    void resetEnvironment();
    void addToEnvironment( QString varAssignment );

    static QString targetName(BuildType buildMode, const QString& target, const QString& version);

    // executes a make clean in the specified workPath
    bool makeClean( const QString &workPath );
    // executes a make dist clean in the specified workPath
    bool makeDistClean( const QString &workPath );
    // executes a qmake -project on the specified workDir
    bool qmakeProject( const QString &workDir, const QString &proName );
    // executes a qmake on proName in the specified workDir, output goes to buildDir or workDir if it's null
    bool qmake(const QString &workDir, const QString &proName, const QString &buildDir = QString(),
               const QStringList &additionalArguments = QStringList());
    // executes qmake in workDir with the specified arguments
    bool qmake(const QString &workDir, const QStringList &arguments);
    // executes a make in the specified workPath, with an optional target (eg. install)
    bool make( const QString &workPath, const QString &target = QString(), bool expectFail = false );
    // checks if the executable exists in destDir
    bool exists(const QString &destDir, const QString &exeName, BuildType buildType,
                const QString &version = QString());
    // removes the makefile
    bool removeMakefile( const QString &workPath );
    // removes the project file specified by 'project' on the 'workPath'
    bool removeProject( const QString &workPath, const QString &project );
    // removes the file specified by 'fileName' on the 'workPath'
    bool removeFile( const QString &workPath, const QString &fileName );

    // Returns each line of stdout/stderr of the last commands
    // separated by platform-specific line endings.
    QString commandOutput() const;

    // clear the results of storage of stdout for running previous commands
    void clearCommandOutput();

    bool runCommand(const QString &cmd, const QStringList &args, bool expectFail = false);

private:
    bool errorOut();

    QString makeCmd_;
    QStringList makeArgs_;
    QString qmakeCmd_;
    QStringList qmakeArgs_;
    QStringList environment_;

    QStringList testOutput_;
};

#endif // TESTCOMPILER_H
