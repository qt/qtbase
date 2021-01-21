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

#ifndef QEVDEVMOUSEHANDLER_P_H
#define QEVDEVMOUSEHANDLER_P_H

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

#include <QObject>
#include <QString>
#include <QPoint>
#include <QEvent>

#include <memory>

QT_BEGIN_NAMESPACE

class QSocketNotifier;

class QEvdevMouseHandler : public QObject
{
    Q_OBJECT
public:
    static std::unique_ptr<QEvdevMouseHandler> create(const QString &device, const QString &specification);
    ~QEvdevMouseHandler();

    void readMouseData();

signals:
    void handleMouseEvent(int x, int y, bool abs, Qt::MouseButtons buttons,
                          Qt::MouseButton button, QEvent::Type type);
    void handleWheelEvent(QPoint delta);

private:
    QEvdevMouseHandler(const QString &device, int fd, bool abs, bool compression, int jitterLimit);

    void sendMouseEvent();
    bool getHardwareMaximum();
    void detectHiResWheelSupport();

    QString m_device;
    int m_fd;
    QSocketNotifier *m_notify = nullptr;
    int m_x = 0, m_y = 0;
    int m_prevx = 0, m_prevy = 0;
    bool m_abs;
    bool m_compression;
    bool m_hiResWheel = false;
    bool m_hiResHWheel = false;
    Qt::MouseButtons m_buttons;
    Qt::MouseButton m_button;
    QEvent::Type m_eventType;
    int m_jitterLimitSquared;
    bool m_prevInvalid = true;
    int m_hardwareWidth;
    int m_hardwareHeight;
    qreal m_hardwareScalerY;
    qreal m_hardwareScalerX;
};

QT_END_NAMESPACE

#endif // QEVDEVMOUSEHANDLER_P_H
