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

#ifndef QLIBINPUTHANDLER_P_H
#define QLIBINPUTHANDLER_P_H

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QMap>

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

struct udev;
struct libinput;
struct libinput_event;

QT_BEGIN_NAMESPACE

class QSocketNotifier;
class QLibInputPointer;
class QLibInputKeyboard;
class QLibInputTouch;

class QLibInputHandler : public QObject
{
public:
    QLibInputHandler(const QString &key, const QString &spec);
    ~QLibInputHandler();

    void onReadyRead();

private:
    void processEvent(libinput_event *ev);

    udev *m_udev;
    libinput *m_li;
    int m_liFd;
    QScopedPointer<QSocketNotifier> m_notifier;
    QScopedPointer<QLibInputPointer> m_pointer;
    QScopedPointer<QLibInputKeyboard> m_keyboard;
    QScopedPointer<QLibInputTouch> m_touch;
    QMap<int, int> m_devCount;
};

QT_END_NAMESPACE

#endif
