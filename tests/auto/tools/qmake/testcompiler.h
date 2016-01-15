/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

    // executes a make clean in the specified workPath
    bool makeClean( const QString &workPath );
    // executes a make dist clean in the specified workPath
    bool makeDistClean( const QString &workPath );
    // executes a qmake -project on the specified workDir
    bool qmakeProject( const QString &workDir, const QString &proName );
    // executes a qmake on proName in the specified workDir, output goes to buildDir or workDir if it's null
    bool qmake( const QString &workDir, const QString &proName, const QString &buildDir = QString() );
    // executes a make in the specified workPath, with an optional target (eg. install)
    bool make( const QString &workPath, const QString &target = QString(), bool expectFail = false );
    // checks if the executable exists in destDir
    bool exists( const QString &destDir, const QString &exeName, BuildType buildType, const QString &version );
    // removes the makefile
    bool removeMakefile( const QString &workPath );
    // removes the project file specified by 'project' on the 'workPath'
    bool removeProject( const QString &workPath, const QString &project );
    // removes the file specified by 'fileName' on the 'workPath'
    bool removeFile( const QString &workPath, const QString &fileName );
    // returns each line of stdout of the last command append with a "new line" character(s) to suit the platform
    QString commandOutput() const;
    // clear the results of storage of stdout for running previous commands
    void clearCommandOutput();

private:
    bool runCommand(const QString &cmd, const QStringList &args, bool expectFail = false);
    bool errorOut();

    QString makeCmd_;
    QStringList makeArgs_;
    QString qmakeCmd_;
    QStringList qmakeArgs_;
    QStringList environment_;

    QStringList testOutput_;
};

#endif // TESTCOMPILER_H
