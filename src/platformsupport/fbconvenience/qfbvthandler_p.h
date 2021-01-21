/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QFBVTHANDLER_H
#define QFBVTHANDLER_H

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
#include <QObject>

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QFbVtHandler : public QObject
{
    Q_OBJECT

public:
    QFbVtHandler(QObject *parent = nullptr);
    ~QFbVtHandler();

signals:
    void interrupted();
    void aboutToSuspend();
    void resumed();

private slots:
    void handleSignal();

private:
    void setKeyboardEnabled(bool enable);
    void handleInt();
    static void signalHandler(int sigNo);

    int m_tty;
    int m_oldKbdMode;
    int m_sigFd[2];
    QSocketNotifier *m_signalNotifier;
};

QT_END_NAMESPACE

#endif
