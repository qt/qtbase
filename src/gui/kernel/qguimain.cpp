/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qguimain.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qguiapplication.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_GUI_MAIN, "qt.gui.main")

#ifdef Q_OS_NACL
// Qt on NaCL startup sequence:
//
// qGuiRegisterAppFunctions is called first, by a global object initializer from
// the application. It registers the application startup and shutdown functions
// but does not call them
//
// qGuiStartup is called second, by the pp::CreateModule() entry point. This
// function creates the QtGui application object.
//
QAppInitFunction g_appInit = 0;
QAppExitFunction g_appExit = 0;
QAppBlockingFunction g_appBlock = 0;

QApplicationConstructurFunction g_applicationConstructorFunction = 0;
QGuiApplication *g_guiApplcation = 0;
QGuiApplication *qGuiApplicationConstructorFunction();

void Q_GUI_EXPORT qGuiRegisterAppFunctions(QAppInitFunction appInitFunction, QAppExitFunction appExitFunction)
{
    qCDebug(QT_GUI_MAIN) << "qGuiRegisterAppFunctions" << appInitFunction << appExitFunction;
    g_appInit = appInitFunction;
    g_appExit = appExitFunction;
    g_applicationConstructorFunction = qGuiApplicationConstructorFunction;
}

void Q_GUI_EXPORT qGuiRegisterAppBlockingFunction(QAppBlockingFunction appBlockingFunction)
{
    qCDebug(QT_GUI_MAIN) << "qGuiRegisterAppBlockingFunction" << appBlockingFunction;
    g_appBlock = appBlockingFunction;
    g_applicationConstructorFunction = qGuiApplicationConstructorFunction;
}

int g_argc = 0;
char *g_argv = 0;

bool qGuiHaveBlockingMain()
{
    return g_appBlock;
}

int qGuiCallBlockingMain()
{
    return g_appBlock(g_argc, &g_argv);
}

QGuiApplication *qGuiApplicationConstructorFunction()
{
    return new QGuiApplication(g_argc, &g_argv);
}

void qGuiStartup()
{
    qCDebug(QT_GUI_MAIN) << "qGuiStartup";
    g_guiApplcation = g_applicationConstructorFunction();
}

void qGuiAppInit()
{
    qCDebug(QT_GUI_MAIN) << "qGuiAppInit";
    if (!g_appBlock)
        g_appInit(g_argc, &g_argv);
}

void qGuiAppExit()
{
    qCDebug(QT_GUI_MAIN) << "qGuiAppExit";
    if (!g_appBlock) {
        g_appExit();
        delete g_guiApplcation;
    }
}

#else

int Q_GUI_EXPORT qGuiMainWithAppFunctions(int argc, char **argv, 
                                            QAppInitFunction appInitFunction,
                                            QAppExitFunction appExitFunction)
{
    QGuiApplication app(argc, argv);
    appInitFunction(argc, argv);
    int returnValue = qApp->exec();
    appExitFunction();
    return returnValue;    
}

#endif

QT_END_NAMESPACE
