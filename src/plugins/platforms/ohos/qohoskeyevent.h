// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSKEYEVENT_H
#define QOHOSKEYEVENT_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

struct QOhosQtKeyEvent
{
    QEvent::Type keyAction;
    Qt::Key keyCode;
    QString keyText;
    Qt::KeyboardModifiers modifiers;
    Qt::KeyboardModifiers guiApplicationKeyboardModifiers;
    quint32 nativeKeyCode;
};

class QOhosKeyEvent
{
public:
    virtual ~QOhosKeyEvent();
    virtual QOhosOptional<QOhosQtKeyEvent> tryConvertToQOhosQtKeyEvent() const = 0;
    virtual bool equals(const QOhosKeyEvent &other) const = 0;

protected:
    QOhosKeyEvent();
};

QT_END_NAMESPACE

#endif // QOHOSKEYEVENT_H
