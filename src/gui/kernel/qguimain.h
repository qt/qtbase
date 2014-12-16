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

#ifndef QT_GUIMAIN
#define QT_GUIMAIN

#include <QtCore/qglobal.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qcoremain.h>

#ifdef Q_OS_NACL
#include <ppapi/cpp/module.h>
#include <QtGui/qpeppermodule.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_GUI_MAIN)

#ifdef Q_OS_NACL

void Q_GUI_EXPORT qGuiRegisterAppFunctions(QAppInitFunction appInitFunction,
                                           QAppExitFunction appExitFunction);

// NaCl QtGui main:
// - define the pp::CreateModule() Ppapi main entry point.
// - register app init and exit functions.
#define Q_GUI_MAIN(qAppInitFunction, qAppExitFunction) \
namespace pp { pp::Module* CreateModule() { return new QPepperModule(); } }\
class QGuiMain \
{ \
public:  \
    QGuiMain() { qGuiRegisterAppFunctions(qAppInitFunction, qAppExitFunction); } \
}; \
QGuiMain qGuiMain;

#else

int Q_GUI_EXPORT qGuiMainWithAppFunctions(int argc, char **argv, 
                                          QAppInitFunction appInitFunction,
                                          QAppExitFunction appExitFunction);

// Standard QtGui main: define main(), call
// qGuiMainWithAppFunctions which will run the applicaiton.
#define Q_GUI_MAIN(qAppInitFunction, qAppExitFunction) \
int main(int argc, char **argv) { \
    return qGuiMainWithAppFunctions( \
        argc, argv, qAppInitFunction, qAppExitFunction); \
}

#endif

QT_END_NAMESPACE

#endif
