// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
