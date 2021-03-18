/***************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHAIKURASTERWINDOW_H
#define QHAIKURASTERWINDOW_H

#include "qhaikuwindow.h"

#include <View.h>

QT_BEGIN_NAMESPACE

class HaikuViewProxy : public QObject, public BView
{
    Q_OBJECT

public:
    explicit HaikuViewProxy(BWindow *window, QObject *parent = nullptr);

    void MessageReceived(BMessage *message) override;
    void Draw(BRect updateRect) override;
    void MouseDown(BPoint pos) override;
    void MouseUp(BPoint pos) override;
    void MouseMoved(BPoint pos, uint32 code, const BMessage *dragMessage) override;
    void KeyDown(const char *bytes, int32 numBytes) override;
    void KeyUp(const char *bytes, int32 numBytes) override;

Q_SIGNALS:
    void mouseEvent(const QPoint &localPosition, const QPoint &globalPosition, Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Qt::MouseEventSource source);
    void wheelEvent(const QPoint &localPosition, const QPoint &globalPosition, int delta, Qt::Orientation orientation, Qt::KeyboardModifiers modifiers);
    void keyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString &text);
    void enteredView();
    void exitedView();
    void drawRequest(const QRect &rect);

private:
    Qt::MouseButtons keyStateToMouseButtons(uint32 keyState) const;
    Qt::KeyboardModifiers keyStateToModifiers(uint32 keyState) const;
    void handleKeyEvent(QEvent::Type type, BMessage *message);
};

class QHaikuRasterWindow : public QHaikuWindow
{
    Q_OBJECT

public:
    explicit QHaikuRasterWindow(QWindow *window);
    ~QHaikuRasterWindow();

    BView* nativeViewHandle() const;

private:
    HaikuViewProxy *m_view;
};

QT_END_NAMESPACE

#endif
