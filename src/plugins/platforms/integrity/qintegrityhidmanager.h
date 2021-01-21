/****************************************************************************
**
** Copyright (C) 2015 Green Hills Software
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QINTEGRITYHIDMANAGER_P_H
#define QINTEGRITYHIDMANAGER_P_H

#include <QObject>
#include <QList>
#include <QThread>

QT_BEGIN_NAMESPACE

class HIDDriverHandler;

class QIntegrityHIDManager : public QThread
{
    Q_OBJECT
public:
    QIntegrityHIDManager(const QString &key, const QString &specification, QObject *parent = 0);
    ~QIntegrityHIDManager();

    void run(void);
private:
    void open_devices(void);

    QString m_spec;
    QList<HIDDriverHandler *> m_drivers;

};

QT_END_NAMESPACE

#endif // QINTEGRITYHIDMANAGER_P_H
