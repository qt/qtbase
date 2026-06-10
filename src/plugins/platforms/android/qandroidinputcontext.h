// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef ANDROIDINPUTCONTEXT_H
#define ANDROIDINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <functional>
#include <jni.h>
#include <qevent.h>

#include <QtCore/qpointer.h>
#include <QTimer>

QT_BEGIN_NAMESPACE

class QAndroidInputContext: public QPlatformInputContext
{
    Q_OBJECT
    enum CapsMode
    {
        CAP_MODE_CHARACTERS = 0x00001000,
        CAP_MODE_SENTENCES  = 0x00004000,
        CAP_MODE_WORDS      = 0x00002000
    };

public:
    enum EditContext : uint32_t {
        CutButton       = 1 << 0,
        CopyButton      = 1 << 1,
        PasteButton     = 1 << 2,
        SelectAllButton = 1 << 3,
        AllButtons      = CutButton | CopyButton | PasteButton | SelectAllButton
    };

    enum HandleMode {
        Hidden        = 0,
        ShowCursor    = 1,
        ShowSelection = 2,
        ShowEditPopup = 0x100
    };
    Q_DECLARE_FLAGS(HandleModes, HandleMode)

    struct ExtractedText
    {
        ExtractedText() { clear(); }

        void clear()
        {
            partialEndOffset = partialStartOffset = selectionEnd = selectionStart = startOffset = -1;
            text.clear();
        }

        int partialEndOffset;
        int partialStartOffset;
        int selectionEnd;
        int selectionStart;
        int startOffset;
        QString text;
    };

public:
    QAndroidInputContext();
    ~QAndroidInputContext();
    static QAndroidInputContext * androidInputContext();
    bool isValid() const override { return true; }

    void setAccessibilityFocusInProgress(bool inProgress) { m_accessibilityFocusInProgress = inProgress; }

    void reset() override;
    void commit() override;
    void update(Qt::InputMethodQueries queries) override;
    void invokeAction(QInputMethod::Action action, int cursorPosition) override;
    QRectF keyboardRect() const override;
    bool isAnimating() const override;
    void showInputPanel() override;
    void hideInputPanel() override;
    bool isInputPanelVisible() const override;

    bool isComposing() const;
    void clear();
    void setFocusObject(QObject *object) override;
    QObject *focusObject();
    void sendShortcut(const QKeySequence &);

    //---------------//
    jboolean beginBatchEdit();
    jboolean endBatchEdit();
    jboolean commitText(const QString &text, jint newCursorPosition);
    jboolean deleteSurroundingText(jint leftLength, jint rightLength);
    jboolean finishComposingText();
    jint getCursorCapsMode(jint reqModes);
    const ExtractedText &getExtractedText(jint hintMaxChars, jint hintMaxLines, jint flags);
    QString getSelectedText(jint flags);
    QString getTextAfterCursor(jint length, jint flags);
    QString getTextBeforeCursor(jint length, jint flags);
    jboolean replaceText(jint start, jint end, const QString text, jint newCursorPosition);
    jboolean setComposingText(const QString &text, jint newCursorPosition);
    jboolean setComposingRegion(jint start, jint end);
    jboolean setSelection(jint start, jint end);
    jboolean selectAll();
    jboolean cut();
    jboolean copy();
    jboolean copyURL();
    jboolean paste();
    void reportFullscreenMode(jboolean enabled);
    jboolean fullscreenMode();

public slots:
    void safeCall(const std::function<void()> &func, Qt::ConnectionType conType = Qt::BlockingQueuedConnection);
    void updateCursorPosition();
    void updateSelectionHandles();
    void handleLocationChanged(int handleId, int x, int y);
    void touchDown(int x, int y);
    void longPress(int x, int y);
    void keyDown();
    void hideSelectionHandles();

private slots:
    void showInputPanelLater(Qt::ApplicationState);

private:
    bool isImhNoTextHandlesSet();
    void sendInputMethodEvent(QInputMethodEvent *event);
    QSharedPointer<QInputMethodQueryEvent> focusObjectInputMethodQuery(Qt::InputMethodQueries queries = Qt::ImQueryAll);
    bool focusObjectIsComposing() const;
    void focusObjectStartComposing();
    bool focusObjectStopComposing();
#if QT_CONFIG(accessibility)
    // Synthesize the Android text-change events QML text items don't emit on the
    // input path, so a screen reader echoes typed/deleted characters. Generated
    // at the platform chokepoints: endBatchEdit() for IME edits, update() for the
    // rest (key events, programmatic changes).
    void notifyTextChangedForAccessibility();
    // Flag a pending edit (and lazily capture the pre-edit baseline) from the IME
    // mutators, so focus-time batch edits don't fire a spurious announcement.
    void markTextEditForAccessibility();
#endif

private:
    ExtractedText m_extractedText;
    QString m_composingText;
    int m_composingTextStart;
    int m_composingCursor;
    QMetaObject::Connection m_updateCursorPosConnection;
    HandleModes m_handleMode;
    int m_batchEditNestingLevel;
#if QT_CONFIG(accessibility)
    // Per-focus text baseline used to diff edits for the screen-reader echo.
    QString m_a11yLastText;
    bool m_a11yTextEditPending = false;
    bool m_a11yBaselineValid = false;
#endif
    QPointer<QObject> m_focusObject;
    QTimer m_hideCursorHandleTimer;
    bool m_fullScreenMode;
    bool m_accessibilityFocusInProgress = false;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QAndroidInputContext::HandleModes)
QT_END_NAMESPACE

#endif // ANDROIDINPUTCONTEXT_H
