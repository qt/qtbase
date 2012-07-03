/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#ifndef QEVDEVTOUCH_P_H
#define QEVDEVTOUCH_P_H

#include <QObject>
#include <QString>
#include <QList>
#include <QThread>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QSocketNotifier;
class QEvdevTouchScreenData;
#ifdef USE_MTDEV
struct mtdev;
#endif

class QEvdevTouchScreenHandler : public QObject
{
    Q_OBJECT

public:
    QEvdevTouchScreenHandler(const QString &spec = QString(), QObject *parent = 0);
    ~QEvdevTouchScreenHandler();

private slots:
    void readData();

private:
    QSocketNotifier *m_notify;
    int m_fd;
    QEvdevTouchScreenData *d;
#ifdef USE_MTDEV
    mtdev *m_mtdev;
#endif
};

class QEvdevTouchScreenHandlerThread : public QThread
{
public:
    QEvdevTouchScreenHandlerThread(const QString &spec, QObject *parent = 0);
    ~QEvdevTouchScreenHandlerThread();
    void run();
    QEvdevTouchScreenHandler *handler() { return m_handler; }

private:
    QString m_spec;
    QEvdevTouchScreenHandler *m_handler;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QEVDEVTOUCH_P_H
