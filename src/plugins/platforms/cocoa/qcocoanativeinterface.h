/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOANATIVEINTERFACE_H
#define QCOCOANATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QWidget;
class QPlatformPrinterSupport;
class QPrintEngine;

class QCocoaNativeInterface : public QPlatformNativeInterface
{
    Q_OBJECT
public:
    QCocoaNativeInterface();

    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window);

public Q_SLOTS:
    void onAppFocusWindowChanged(QWindow *window);

private:
    /*
        "Virtual" function to create the platform printer support
        implementation.

        We use an invokable function instead of a virtual one, we do not want
        this in the QPlatform* API yet.

        This was added here only because QPlatformNativeInterface is a QObject
        and allow us to use QMetaObject::indexOfMethod() from the printsupport
        plugin.
    */
    Q_INVOKABLE QPlatformPrinterSupport *createPlatformPrinterSupport();
    /*
        Function to return the NSPrintInfo * from QMacPaintEnginePrivate.
        Needed by the native print dialog in the QtPrintSupport library.
    */
    Q_INVOKABLE void *NSPrintInfoForPrintEngine(QPrintEngine *printEngine);
};

#endif // QCOCOANATIVEINTERFACE_H

QT_END_NAMESPACE
