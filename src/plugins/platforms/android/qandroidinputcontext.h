/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
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

#ifndef ANDROIDINPUTCONTEXT_H
#define ANDROIDINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <functional>
#include <jni.h>
#include <qevent.h>
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
    jboolean setComposingText(const QString &text, jint newCursorPosition);
    jboolean setComposingRegion(jint start, jint end);
    jboolean setSelection(jint start, jint end);
    jboolean selectAll();
    jboolean cut();
    jboolean copy();
    jboolean copyURL();
    jboolean paste();

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
    void sendInputMethodEvent(QInputMethodEvent *event);
    QSharedPointer<QInputMethodQueryEvent> focusObjectInputMethodQuery(Qt::InputMethodQueries queries = Qt::ImQueryAll);
    bool focusObjectIsComposing() const;
    void focusObjectStartComposing();
    bool focusObjectStopComposing();

private:
    ExtractedText m_extractedText;
    QString m_composingText;
    int m_composingTextStart;
    int m_composingCursor;
    QMetaObject::Connection m_updateCursorPosConnection;
    HandleModes m_handleMode;
    int m_batchEditNestingLevel;
    QObject *m_focusObject;
    QTimer m_hideCursorHandleTimer;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QAndroidInputContext::HandleModes)
QT_END_NAMESPACE

#endif // ANDROIDINPUTCONTEXT_H
