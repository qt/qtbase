// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLIBINPUTHANDLER_P_H
#define QLIBINPUTHANDLER_P_H

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>
#include <QtCore/QMap>
#include <QtCore/private/qglobal_p.h>

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
