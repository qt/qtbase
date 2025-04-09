// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.util.Log;
import android.view.inputmethod.TextAttribute;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;

class QtExtractedText
{
    int partialEndOffset;
    int partialStartOffset;
    int selectionEnd;
    int selectionStart;
    int startOffset;
    String text;
}

class QtNativeInputConnection
{
    static native boolean beginBatchEdit();
    static native boolean endBatchEdit();
    static native boolean commitText(String text, int newCursorPosition);
    static native boolean commitCompletion(String text, int position);
    static native boolean deleteSurroundingText(int leftLength, int rightLength);
    static native boolean finishComposingText();
    static native int getCursorCapsMode(int reqModes);
    static native QtExtractedText getExtractedText(int hintMaxChars, int hintMaxLines, int flags);
    static native String getSelectedText(int flags);
    static native String getTextAfterCursor(int length, int flags);
    static native String getTextBeforeCursor(int length, int flags);
    static native boolean replaceText(int start, int end, String text, int newCursorPosition);
    static native boolean setComposingText(String text, int newCursorPosition);
    static native boolean setComposingRegion(int start, int end);
    static native boolean setSelection(int start, int end);
    static native boolean selectAll();
    static native boolean cut();
    static native boolean copy();
    static native boolean copyURL();
    static native boolean paste();
    static native boolean updateCursorPosition();
    static native void reportFullscreenMode(boolean enabled);
    static native boolean fullscreenMode();
}

class QtInputConnection extends BaseInputConnection
{
    private static final int ID_SELECT_ALL = android.R.id.selectAll;
    private static final int ID_CUT = android.R.id.cut;
    private static final int ID_COPY = android.R.id.copy;
    private static final int ID_PASTE = android.R.id.paste;
    private static final int ID_COPY_URL = android.R.id.copyUrl;
    private static final int ID_SWITCH_INPUT_METHOD = android.R.id.switchInputMethod;
    private static final int ID_ADD_TO_DICTIONARY = android.R.id.addToDictionary;
    private static final int KEYBOARD_CHECK_DELAY_MS = 100;

    private static final String QtTAG = "QtInputConnection";

    private boolean m_duringBatchEdit = false;
    private final QtInputConnectionListener m_qtInputConnectionListener;

    class HideKeyboardRunnable implements Runnable {
        private int m_numberOfAttempts = 10;

        @Override
        public void run() {
            // Check that the keyboard is really no longer there.
            if (m_qtInputConnectionListener == null) {
                Log.w(QtTAG, "HideKeyboardRunnable: QtInputConnectionListener is null");
                return;
            }

            if (m_qtInputConnectionListener.keyboardTransitionInProgress()
                    && m_numberOfAttempts > 0) {
                --m_numberOfAttempts;
                m_view.postDelayed(this, KEYBOARD_CHECK_DELAY_MS);
                return;
            }

            if (m_qtInputConnectionListener.isKeyboardHidden())
                m_qtInputConnectionListener.onHideKeyboardRunnableDone(false, System.nanoTime());
        }
    }

    interface QtInputConnectionListener {
        void onSetClosing(boolean closing);
        void onHideKeyboardRunnableDone(boolean visibility, long hideTimeStamp);
        void onSendKeyEventDefaultCase();
        void onEditTextChanged(QtEditText editText);
        boolean keyboardTransitionInProgress();
        boolean isKeyboardHidden();
    }

    private final QtEditText m_view;
    private final InputMethodManager m_imm;

    private void setClosing(boolean closing)
    {
        if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            if (closing)
                m_view.postDelayed(new HideKeyboardRunnable(), KEYBOARD_CHECK_DELAY_MS);
            else if (m_qtInputConnectionListener != null)
                m_qtInputConnectionListener.onSetClosing(false);
        }
    }

    QtInputConnection(QtEditText targetView, QtInputConnectionListener listener)
    {
        super(targetView, true);
        m_view = targetView;
        m_imm = (InputMethodManager)m_view.getContext().getSystemService(
                                        Context.INPUT_METHOD_SERVICE);
        m_qtInputConnectionListener = listener;
    }

    void restartImmInput()
    {
        if (QtNativeInputConnection.fullscreenMode() && !m_duringBatchEdit) {
            if (m_imm != null)
                m_imm.restartInput(m_view);
        }

    }

    @Override
    public boolean beginBatchEdit()
    {
        setClosing(false);
        m_duringBatchEdit = true;
        return QtNativeInputConnection.beginBatchEdit();
    }

    @Override
    public boolean reportFullscreenMode (boolean enabled)
    {
        QtNativeInputConnection.reportFullscreenMode(enabled);
        // Always ignored on calling editor.
        // Always false on Android 8 and later, true with earlier.
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.O;
    }

    @Override
    public boolean endBatchEdit()
    {
        setClosing(false);
        boolean ret = QtNativeInputConnection.endBatchEdit();
        if (m_duringBatchEdit) {
            m_duringBatchEdit = false;
            restartImmInput();
        }
        return ret;
    }

    @Override
    public boolean commitCompletion(CompletionInfo text)
    {
        setClosing(false);
        return QtNativeInputConnection.commitCompletion(text.getText().toString(), text.getPosition());
    }

    @Override
    public boolean commitText(CharSequence text, int newCursorPosition)
    {
        setClosing(false);
        boolean result = QtNativeInputConnection.commitText(text.toString(), newCursorPosition);
        restartImmInput();
        return result;
    }

    @Override
    public boolean deleteSurroundingText(int leftLength, int rightLength)
    {
        setClosing(false);
        boolean result = QtNativeInputConnection.deleteSurroundingText(leftLength, rightLength);
        restartImmInput();
        return result;
    }

    @Override
    public boolean finishComposingText()
    {
        // on some/all android devices hide event is not coming, but instead finishComposingText() is called twice
        setClosing(true);
        return QtNativeInputConnection.finishComposingText();
    }

    @Override
    public int getCursorCapsMode(int reqModes)
    {
        return QtNativeInputConnection.getCursorCapsMode(reqModes);
    }

    @Override
    public ExtractedText getExtractedText(ExtractedTextRequest request, int flags)
    {
        QtExtractedText qExtractedText = QtNativeInputConnection.getExtractedText(request.hintMaxChars,
                                                                                  request.hintMaxLines,
                                                                                  flags);
        if (qExtractedText == null)
            return null;

        ExtractedText extractedText = new ExtractedText();
        extractedText.partialEndOffset = qExtractedText.partialEndOffset;
        extractedText.partialStartOffset = qExtractedText.partialStartOffset;
        extractedText.selectionEnd = qExtractedText.selectionEnd;
        extractedText.selectionStart = qExtractedText.selectionStart;
        extractedText.startOffset = qExtractedText.startOffset;
        extractedText.text = qExtractedText.text;
        return extractedText;
    }

    public CharSequence getSelectedText(int flags)
    {
        return QtNativeInputConnection.getSelectedText(flags);
    }

    @Override
    public CharSequence getTextAfterCursor(int length, int flags)
    {
        return QtNativeInputConnection.getTextAfterCursor(length, flags);
    }

    @Override
    public CharSequence getTextBeforeCursor(int length, int flags)
    {
        return QtNativeInputConnection.getTextBeforeCursor(length, flags);
    }

    @Override
    public boolean performContextMenuAction(int id)
    {
        switch (id) {
        case ID_SELECT_ALL:
            restartImmInput();
            return QtNativeInputConnection.selectAll();
        case ID_COPY:
            restartImmInput();
            return QtNativeInputConnection.copy();
        case ID_COPY_URL:
            restartImmInput();
            return QtNativeInputConnection.copyURL();
        case ID_CUT:
            restartImmInput();
            return QtNativeInputConnection.cut();
        case ID_PASTE:
            restartImmInput();
            return QtNativeInputConnection.paste();
        case ID_SWITCH_INPUT_METHOD:
            if (m_imm != null)
                m_imm.showInputMethodPicker();

            return true;
        case ID_ADD_TO_DICTIONARY:
// TODO
//            String word = m_editable.subSequence(0, m_editable.length()).toString();
//            if (word != null) {
//                Intent i = new Intent("com.android.settings.USER_DICTIONARY_INSERT");
//                i.putExtra("word", word);
//                i.setFlags(i.getFlags() | Intent.FLAG_ACTIVITY_NEW_TASK);
//                m_view.getContext().startActivity(i);
//            }
            return true;
        }
        return super.performContextMenuAction(id);
    }

    @Override
    public boolean sendKeyEvent(KeyEvent event)
    {
        // QTBUG-85715
        // If the sendKeyEvent was invoked, it means that the button not related with composingText was used
        // In such case composing text (if it exists) should be finished immediately
        finishComposingText();
        if (event.getKeyCode() == KeyEvent.KEYCODE_ENTER && m_view != null) {
            KeyEvent fakeEvent;
            switch (m_view.m_imeOptions) {
                case android.view.inputmethod.EditorInfo.IME_ACTION_NEXT:
                    fakeEvent = new KeyEvent(event.getDownTime(),
                                            event.getEventTime(),
                                            event.getAction(),
                                            KeyEvent.KEYCODE_TAB,
                                            event.getRepeatCount(),
                                            event.getMetaState());
                    return super.sendKeyEvent(fakeEvent);
                case android.view.inputmethod.EditorInfo.IME_ACTION_PREVIOUS:
                    fakeEvent = new KeyEvent(event.getDownTime(),
                                            event.getEventTime(),
                                            event.getAction(),
                                            KeyEvent.KEYCODE_TAB,
                                            event.getRepeatCount(),
                                            KeyEvent.META_SHIFT_ON);
                    return super.sendKeyEvent(fakeEvent);
                case android.view.inputmethod.EditorInfo.IME_FLAG_NO_ENTER_ACTION:
                    restartImmInput();
                    break;
                default:
                    if (m_qtInputConnectionListener != null)
                        m_qtInputConnectionListener.onSendKeyEventDefaultCase();
                    break;
            }
        }
        return super.sendKeyEvent(event);
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition)
    {
        setClosing(false);
        boolean result = QtNativeInputConnection.setComposingText(text.toString(), newCursorPosition);
        restartImmInput();
        return result;
    }

    @TargetApi(33)
    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition, TextAttribute textAttribute)
    {
        return setComposingText(text, newCursorPosition);
    }

    @TargetApi(33)
    @Override
    public boolean setComposingRegion(int start, int end, TextAttribute textAttribute)
    {
        return setComposingRegion(start, end);
    }

    @TargetApi(33)
    @Override
    public boolean commitText(CharSequence text, int newCursorPosition, TextAttribute textAttribute)
    {
        return commitText(text, newCursorPosition);
    }

    @TargetApi(34)
    @Override
    public boolean replaceText(int start, int end, CharSequence text, int newCursorPosition, TextAttribute textAttribute)
    {
        setClosing(false);
        return QtNativeInputConnection.replaceText(start, end, text.toString(), newCursorPosition);
    }

    @Override
    public boolean setComposingRegion(int start, int end)
    {
        setClosing(false);
        return QtNativeInputConnection.setComposingRegion(start, end);
    }

    @Override
    public boolean setSelection(int start, int end)
    {
        setClosing(false);
        boolean result = QtNativeInputConnection.setSelection(start, end);
        restartImmInput();
        return result;
    }
}
