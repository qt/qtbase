// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTSLIB_H
#define QTSLIB_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QObject>
#include <private/qglobal_p.h>

struct tsdev;

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QTsLibMouseHandler : public QObject
{
    Q_OBJECT

public:
    QTsLibMouseHandler(const QString &key, const QString &specification, QObject *parent = nullptr);
    ~QTsLibMouseHandler();

private slots:
    void readMouseData();

private:
    QSocketNotifier * m_notify = nullptr;
    tsdev *m_dev;
    int m_x = 0;
    int m_y = 0;
    bool m_pressed = false;
    const bool m_rawMode;
};

QT_END_NAMESPACE

#endif // QTSLIB_H
