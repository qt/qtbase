/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef QWINDOWSINPUTCONTEXT_H
#define QWINDOWSINPUTCONTEXT_H

#include <QtCore/qt_windows.h>

#include <QtCore/QLocale>
#include <QtCore/QPointer>
#include <qpa/qplatforminputcontext.h>

QT_BEGIN_NAMESPACE

class QInputMethodEvent;
class QWindowsWindow;

class QWindowsInputContext : public QPlatformInputContext
{
    Q_OBJECT

    struct CompositionContext
    {
        HWND hwnd = 0;
        QString composition;
        int position = 0;
        bool isComposing = false;
        QPointer<QObject> focusObject;
    };
public:
    explicit QWindowsInputContext();
    ~QWindowsInputContext();

    static void setWindowsImeEnabled(QWindowsWindow *platformWindow, bool enabled);

    bool hasCapability(Capability capability) const override;
    QLocale locale() const override { return m_locale; }

    void reset() override;
    void update(Qt::InputMethodQueries) override;
    void invokeAction(QInputMethod::Action, int cursorPosition) override;
    void setFocusObject(QObject *object) override;

    QRectF keyboardRect() const override;
    bool isInputPanelVisible() const override;
    void showInputPanel() override;
    void hideInputPanel() override;

    bool startComposition(HWND hwnd);
    bool composition(HWND hwnd, LPARAM lParam);
    bool endComposition(HWND hwnd);
    inline bool isComposing() const { return m_compositionContext.isComposing; }

    int reconvertString(RECONVERTSTRING *reconv);

    bool handleIME_Request(WPARAM wparam, LPARAM lparam, LRESULT *result);
    void handleInputLanguageChanged(WPARAM wparam, LPARAM lparam);

private slots:
    void cursorRectChanged();

private:
    void initContext(HWND hwnd, QObject *focusObject);
    void doneContext();
    void startContextComposition();
    void endContextComposition();
    void updateEnabled();
    HWND getVirtualKeyboardWindowHandle() const;

    const DWORD m_WM_MSIME_MOUSE;
    bool m_caretCreated = false;
    HBITMAP m_transparentBitmap;
    CompositionContext m_compositionContext;
    bool m_endCompositionRecursionGuard = false;
    LCID m_languageId;
    QLocale m_locale;
};

QT_END_NAMESPACE

#endif // QWINDOWSINPUTCONTEXT_H
