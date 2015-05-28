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

#ifndef QT_CORE_MAIN
#define QT_CORE_MAIN

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

typedef void (*QAppInitFunction)(int argc, char **argv);
typedef void (*QAppExitFunction)();
typedef int (*QAppBlockingFunction)(int argc, char **argv);

int Q_CORE_EXPORT qCoreMainWithAppFunctions(int argc, char **argv,
                                            QAppInitFunction appInitFunction,
                                            QAppExitFunction appExitFunction);

int Q_CORE_EXPORT qCoreMainWithBlockingFunction(int argc, char **argv,
                                                QAppBlockingFunction appBlockingFunction);


#ifdef Q_OS_NACL

#define Q_CORE_MAIN(qAppInitFunction, qAppExitFunction) \
\
namespace pp { class Module; } \
extern pp::Module *qtCoreCreatePepperModule(); \
namespace pp {  \
    pp::Module* CreateModule() { \
        return qtCoreCreatePepperModule(); \
    } \
\
int main(int argc, char **argv) { \
    return qCoreMainWithAppFunctions( \
        argc, argv, qAppInitFunction, qAppExitFunction); \
}

#define Q_CORE_BLOCKING_MAIN(appBlockingFunction) \
\
namespace pp { class Module; } \
extern pp::Module *qtCoreCreatePepperModule(); \
namespace pp { \
    pp::Module* CreateModule() { \
        return qtCoreCreatePepperModule(); \
    } \
} \
\
int main(int argc, char **argv) { \
    return qCoreMainWithBlockingFunction(argc, argv, appBlockingFunction); \
}

#else

#define Q_CORE_MAIN(qAppInitFunction, qAppExitFunction) \
int main(int argc, char **argv) { \
    return qCoreMainWithAppFunctions( \
        argc, argv, qAppInitFunction, qAppExitFunction); \
}

#define Q_CORE_BLOCKING_MAIN(appBlockingFunction) \
int main(int argc, char **argv) { \
    return qCoreMainWithBlockingFunction(argc, argv, appBlockingFunction); \
}

#endif

QT_END_NAMESPACE

#endif
