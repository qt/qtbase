/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "testcompiler.h"

#include <QProcess>
#include <QDir>

static QString targetName( BuildType buildMode, const QString& target, const QString& version )
{
    Q_UNUSED(version);
    QString targetName = target;

#if defined (Q_OS_WIN32)
    switch (buildMode)
    {
    case Exe: // app
        targetName.append(".exe");
        break;
    case Dll: // dll
        if (!version.isEmpty())
            targetName.append(version.section(".", 0, 0));
        targetName.append(".dll");
        break;
    case Lib: // lib
#ifdef Q_CC_GNU
        targetName.prepend("lib");
        targetName.append(".a");
#else
        targetName.append(".lib");
#endif
        break;
    case Plain:
        // no conversion
        break;
    }
#elif defined( Q_OS_MAC )
    switch (buildMode)
    {
    case Exe: // app
        targetName += ".app/Contents/MacOS/" + target.section('/', -1);
        break;
    case Dll: // dll
        targetName.prepend("lib");
        targetName.append("." + version + ".dylib");
        break;
    case Lib: // lib
        targetName.prepend("lib");
        targetName.append(".a");
        break;
    case Plain:
        // no conversion
        break;
    }
#else
    switch (buildMode)
    {
    case Exe: // app
        break;
    case Dll: // dll
        targetName.prepend("lib");
#if defined (Q_OS_HPUX) && !defined (__ia64)
        targetName.append(".sl");
#elif defined (Q_OS_AIX)
        targetName.append(".a");
#else
        targetName.append(".so");
#endif
        break;
    case Lib: // lib
        targetName.prepend("lib");
        targetName.append(".a");
        break;
    case Plain:
        // no conversion
        break;
    }
#endif
    return targetName;
}

TestCompiler::TestCompiler()
{
    setBaseCommands( "", "" );
}

TestCompiler::~TestCompiler()
{
}

bool TestCompiler::runCommand( QString cmdline )
{
    testOutput_.append("Running command: " + cmdline);

    QProcess child;
    if (!environment_.empty())
        child.setEnvironment(QProcess::systemEnvironment() + environment_);

    child.start(cmdline);
    if (!child.waitForStarted(-1)) {
        testOutput_.append( "Unable to start child process." );
        return false;
    }

    bool failed = false;
    child.setReadChannel(QProcess::StandardError);
    child.waitForFinished(-1);

    foreach (const QByteArray &output, child.readAllStandardError().split('\n')) {
        testOutput_.append(QString::fromLocal8Bit(output));

        if (output.startsWith("Project MESSAGE: FAILED"))
            failed = true;
    }

    return !failed && child.exitStatus() == QProcess::NormalExit && child.exitCode() == 0;
}

void TestCompiler::setBaseCommands( QString makeCmd, QString qmakeCmd )
{
    makeCmd_ = makeCmd;
    qmakeCmd_ = qmakeCmd;
}

void TestCompiler::resetEnvironment()
{
    environment_.clear();
}

void TestCompiler::addToEnvironment( QString varAssignment )
{
    environment_.push_back(varAssignment);
}

bool TestCompiler::makeClean( const QString &workPath )
{
    QDir D;
    if (!D.exists(workPath)) {
        testOutput_.append( "Directory '" + workPath + "' doesn't exist" );
        return false;
    }

    D.setCurrent(workPath);
    QFileInfo Fi( workPath + "/Makefile");
    if (Fi.exists())
        // Run make clean
        return runCommand( makeCmd_ + " clean" );

    return true;
}

bool TestCompiler::makeDistClean( const QString &workPath )
{
    QDir D;
    if (!D.exists(workPath)) {
        testOutput_.append( "Directory '" + workPath + "' doesn't exist" );
        return false;
    }

    D.setCurrent(workPath);
    QFileInfo Fi( workPath + "/Makefile");
    if (Fi.exists())
        // Run make distclean
        return runCommand( makeCmd_ + " distclean" );

    return true;

}

bool TestCompiler::qmake( const QString &workDir, const QString &proName, const QString &buildDir )
{
    QDir D;
    D.setCurrent( workDir );

    if (D.exists("Makefile"))
        D.remove("Makefile");

    QString projectFile = proName;
    QString makeFile = buildDir;
    if (!projectFile.endsWith(".pro"))
        projectFile += ".pro";
    if (!makeFile.isEmpty() && !makeFile.endsWith('/'))
        makeFile += '/';
    makeFile += "Makefile";

    // Now start qmake and generate the makefile
    return runCommand( qmakeCmd_ + " " + projectFile + " -o " + makeFile );
}

bool TestCompiler::make( const QString &workPath, const QString &target )
{
    QDir D;
    D.setCurrent( workPath );

    QString cmdline = makeCmd_;
    if ( cmdline.contains("nmake", Qt::CaseInsensitive) )
        cmdline.append(" /NOLOGO");
    if ( !target.isEmpty() )
        cmdline += " " + target;

    return runCommand( cmdline );
}

bool TestCompiler::exists( const QString &destDir, const QString &exeName, BuildType buildType, const QString &version )
{
    QFileInfo f(destDir + "/" + targetName(buildType, exeName, version));
    return f.exists();
}

bool TestCompiler::removeMakefile( const QString &workPath )
{
    QDir D;
    D.setCurrent( workPath );
    if ( D.exists( "Makefile" ) )
        return D.remove( "Makefile" );
    else
        return true;
}

QString TestCompiler::commandOutput() const
{
#ifndef Q_OS_WIN
    return testOutput_.join("\n");
#else
    return testOutput_.join("\r\n");
#endif
}

void TestCompiler::clearCommandOutput()
{
   testOutput_.clear();
}
