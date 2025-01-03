// Copyright (C) 2015-2016 Oleksandr Tymoshenko <gonzo@bluezbox.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBSDMOUSE_H
#define QBSDMOUSE_H

#include <QString>
#include <QScopedPointer>
#include <QSocketNotifier>

#include <qobject.h>

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QBsdMouseHandler : public QObject
{
    Q_OBJECT
public:
    QBsdMouseHandler(const QString &key, const QString &specification);
    ~QBsdMouseHandler() override;

private:
    void readMouseData();

private:
    QScopedPointer<QSocketNotifier> m_notifier;
    int m_devFd = -1;
    int m_packetSize = 0;
    int m_x = 0;
    int m_y = 0;
    int m_xOffset = 0;
    int m_yOffset = 0;
    Qt::MouseButtons m_buttons = Qt::NoButton;
};

QT_END_NAMESPACE

#endif // QBSDMOUSE_H
