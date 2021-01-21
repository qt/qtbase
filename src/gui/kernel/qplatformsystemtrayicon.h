/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2012 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Christoph Schleifenbaum <christoph.schleifenbaum@kdab.com>
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

#ifndef QPLATFORMSYSTEMTRAYICON_H
#define QPLATFORMSYSTEMTRAYICON_H

#include <QtGui/qtguiglobal.h>
#include "QtCore/qobject.h"

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE

class QPlatformMenu;
class QPlatformScreen;
class QIcon;
class QString;
class QRect;

class Q_GUI_EXPORT QPlatformSystemTrayIcon : public QObject
{
    Q_OBJECT
public:
    enum ActivationReason {
        Unknown,
        Context,
        DoubleClick,
        Trigger,
        MiddleClick
    };
    Q_ENUM(ActivationReason)

    enum MessageIcon { NoIcon, Information, Warning, Critical };
    Q_ENUM(MessageIcon)

    QPlatformSystemTrayIcon();
    ~QPlatformSystemTrayIcon();

    virtual void init() = 0;
    virtual void cleanup() = 0;
    virtual void updateIcon(const QIcon &icon) = 0;
    virtual void updateToolTip(const QString &tooltip) = 0;
    virtual void updateMenu(QPlatformMenu *menu) = 0;
    virtual QRect geometry() const = 0;
    virtual void showMessage(const QString &title, const QString &msg,
                             const QIcon &icon, MessageIcon iconType, int msecs) = 0;

    virtual bool isSystemTrayAvailable() const = 0;
    virtual bool supportsMessages() const = 0;

    virtual QPlatformMenu *createMenu() const;

Q_SIGNALS:
    void activated(QPlatformSystemTrayIcon::ActivationReason reason);
    void contextMenuRequested(QPoint globalPos, const QPlatformScreen *screen);
    void messageClicked();
};

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON

#endif // QSYSTEMTRAYICON_P_H
