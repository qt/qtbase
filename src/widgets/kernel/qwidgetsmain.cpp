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
#include "qwidgetsmain.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qguimain.h>
#include <QtWidgets/qapplication.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_WIDGETS_MAIN, "qt.widgets.main")

#ifdef Q_OS_NACL

extern QApplicationConstructurFunction g_applicationConstructorFunction;
extern int g_argc;
extern char *g_argv;

QGuiApplication *qWidgetApplicationConstructorFunction()
{
    return new QApplication(g_argc, &g_argv);
}

void Q_WIDGETS_EXPORT qWidgetsRegisterAppFunctions(QAppInitFunction appInitFunction, QAppExitFunction appExitFunction)
{
    qCDebug(QT_WIDGETS_MAIN) << "qWidgetsRegisterAppFunctions" << appInitFunction << appExitFunction;
    qGuiRegisterAppFunctions(appInitFunction, appExitFunction);
    g_applicationConstructorFunction = qWidgetApplicationConstructorFunction;
}

void Q_WIDGETS_EXPORT qWidgetsRegisterAppBlockingFunction(QAppBlockingFunction appBlockingFunction)
{
    qCDebug(QT_WIDGETS_MAIN) << "qWidgetsRegisterAppBlockingFunction" << appBlockingFunction;
    qGuiRegisterAppBlockingFunction(appBlockingFunction);
    g_applicationConstructorFunction = qWidgetApplicationConstructorFunction;
}

#else

int Q_WIDGETS_EXPORT qWidgetsMainWithAppFunctions(int argc, char **argv, 
                                                  QAppInitFunction appInitFunction,
                                                  QAppExitFunction appExitFunction)
{
    QApplication app(argc, argv);
    appInitFunction(argc, argv);
    int returnValue = qApp->exec();
    appExitFunction();
    return returnValue;    
}

#endif

QT_END_NAMESPACE
