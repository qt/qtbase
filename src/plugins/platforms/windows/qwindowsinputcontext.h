// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSINPUTCONTEXT_H
#define QWINDOWSINPUTCONTEXT_H

#include <QtCore/qt_windows.h>

#include <QtCore/qlocale.h>
#include <QtCore/qpointer.h>
#include <qpa/qplatforminputcontext.h>

QT_BEGIN_NAMESPACE

class QInputMethodEvent;
class QWindowsWindow;

class QWindowsInputContext : public QPlatformInputContext
{
    Q_DISABLE_COPY_MOVE(QWindowsInputContext)
    Q_OBJECT

    struct CompositionContext
    {
        HWND hwnd = nullptr;
        QString composition;
        int position = 0;
        bool isComposing = false;
        QPointer<QObject> focusObject;
    };
public:
    explicit QWindowsInputContext();
    ~QWindowsInputContext() override;

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
