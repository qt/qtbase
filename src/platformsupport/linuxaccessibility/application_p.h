/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef Q_SPI_APPLICATION_H
#define Q_SPI_APPLICATION_H

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

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/QPointer>
#include <QtCore/QQueue>
#include <QtDBus/QDBusConnection>
#include <QtGui/QAccessibleInterface>

QT_REQUIRE_CONFIG(accessibility);

QT_BEGIN_NAMESPACE

/*
 * Used for the root object.
 *
 * Uses the root object reference and reports its parent as the desktop object.
 */
class QSpiApplicationAdaptor :public QObject
{
    Q_OBJECT

public:
    QSpiApplicationAdaptor(const QDBusConnection &connection, QObject *parent);
    virtual ~QSpiApplicationAdaptor() {}
    void sendEvents(bool active);

Q_SIGNALS:
    void windowActivated(QObject* window, bool active);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private Q_SLOTS:
    void notifyKeyboardListenerCallback(const QDBusMessage& message);
    void notifyKeyboardListenerError(const QDBusError& error, const QDBusMessage& message);

private:
    static QKeyEvent* copyKeyEvent(QKeyEvent*);

    QQueue<QPair<QPointer<QObject>, QKeyEvent*> > keyEvents;
    QDBusConnection dbusConnection;
    bool inCapsLock;
};

QT_END_NAMESPACE

#endif
