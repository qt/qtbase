/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QXCBNATIVEINTERFACEHANDLER_H
#define QXCBNATIVEINTERFACEHANDLER_H

#include <QtCore/QByteArray>
#include <QtGui/qpa/qplatformnativeinterface.h>

#include "qxcbexport.h"

QT_BEGIN_NAMESPACE

class QXcbNativeInterface;
class Q_XCB_EXPORT QXcbNativeInterfaceHandler
{
public:
    QXcbNativeInterfaceHandler(QXcbNativeInterface *nativeInterface);
    virtual ~QXcbNativeInterfaceHandler();

    virtual QPlatformNativeInterface::NativeResourceForIntegrationFunction nativeResourceFunctionForIntegration(const QByteArray &resource) const;
    virtual QPlatformNativeInterface::NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) const;
    virtual QPlatformNativeInterface::NativeResourceForScreenFunction nativeResourceFunctionForScreen(const QByteArray &resource) const;
    virtual QPlatformNativeInterface::NativeResourceForWindowFunction nativeResourceFunctionForWindow(const QByteArray &resource) const;
    virtual QPlatformNativeInterface::NativeResourceForBackingStoreFunction nativeResourceFunctionForBackingStore(const QByteArray &resource) const;

    virtual QFunctionPointer platformFunction(const QByteArray &function) const;
protected:
    QXcbNativeInterface *m_native_interface;
};

QT_END_NAMESPACE

#endif //QXCBNATIVEINTERFACEHANDLER_H
