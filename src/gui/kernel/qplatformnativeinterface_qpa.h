/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPLATFORMNATIVEINTERFACE_QPA_H
#define QPLATFORMNATIVEINTERFACE_QPA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qwindowdefs.h>
#include <QtCore/QObject>
#include <QtCore/QVariant>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QOpenGLContext;
class QWindow;
class QPlatformWindow;
class QBackingStore;

class Q_GUI_EXPORT QPlatformNativeInterface : public QObject
{
    Q_OBJECT
public:
    virtual void *nativeResourceForIntegration(const QByteArray &resource);
    virtual void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context);
    virtual void *nativeResourceForWindow(const QByteArray &resource, QWindow *window);
    virtual void *nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *backingStore);

    virtual QVariantMap windowProperties(QPlatformWindow *window) const;
    virtual QVariant windowProperty(QPlatformWindow *window, const QString &name) const;
    virtual QVariant windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const;
    virtual void setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value);

    typedef bool (*EventFilter)(void *message, long *result);
    virtual EventFilter setEventFilter(const QByteArray &eventType, EventFilter filter);

Q_SIGNALS:
    void windowPropertyChanged(QPlatformWindow *window, const QString &propertyName);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPLATFORMNATIVEINTERFACE_QPA_H
