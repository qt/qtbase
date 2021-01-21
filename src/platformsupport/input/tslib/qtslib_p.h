/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

struct tsdev;

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QTsLibMouseHandler : public QObject
{
    Q_OBJECT

public:
    QTsLibMouseHandler(const QString &key, const QString &specification, QObject *parent = 0);
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
