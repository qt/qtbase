// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.Context;
import android.os.Build;
import android.view.WindowMetrics;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.ExtractedText;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputMethodManager;
import android.view.KeyEvent;
import android.graphics.Rect;
import android.app.Activity;
import android.util.DisplayMetrics;

class QtExtractedText
{
    public int partialEndOffset;
    public int partialStartOffset;
    public int selectionEnd;
    public int selectionStart;
    public int startOffset;
    public String text;
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
    static native boolean setComposingText(String text, int newCursorPosition);
    static native boolean setComposingRegion(int start, int end);
    static native boolean setSelection(int start, int end);
    static native boolean selectAll();
    static native boolean cut();
    static native boolean copy();
    static native boolean copyURL();
    static native boolean paste();
    static native boolean updateCursorPosition();
}

class HideKeyboardRunnable implements Runnable {
    private long m_hideTimeStamp = System.nanoTime();

    @Override
    public void run() {
        // Check that the keyboard is really no longer there.
        Activity activity = QtNative.activity();
        Rect r = new Rect();
        activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);

        int screenHeight = 0;
        if (android.os.Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            DisplayMetrics metrics = new DisplayMetrics();
            activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            screenHeight = metrics.heightPixels;
        } else {
            final WindowMetrics maximumWindowMetrics = activity.getWindowManager().getMaximumWindowMetrics();
            screenHeight = maximumWindowMetrics.getBounds().height();
        }
        final int kbHeight = screenHeight - r.bottom;
        if (kbHeight < 100)
            QtNative.activityDelegate().setKeyboardVisibility(false, m_hideTimeStamp);
    }
}

public class QtInputConnection extends BaseInputConnection
{
    private static final int ID_SELECT_ALL = android.R.id.selectAll;
    private static final int ID_CUT = android.R.id.cut;
    private static final int ID_COPY = android.R.id.copy;
    private static final int ID_PASTE = android.R.id.paste;
    private static final int ID_COPY_URL = android.R.id.copyUrl;
    private static final int ID_SWITCH_INPUT_METHOD = android.R.id.switchInputMethod;
    private static final int ID_ADD_TO_DICTIONARY = android.R.id.addToDictionary;

    private QtEditText m_view = null;

    private void setClosing(boolean closing)
    {
        if (closing) {
            m_view.postDelayed(new HideKeyboardRunnable(), 100);
        } else {
            QtNative.activityDelegate().setKeyboardVisibility(true, System.nanoTime());
        }
    }

    public QtInputConnection(QtEditText targetView)
    {
        super(targetView, true);
        m_view = targetView;
    }

    @Override
    public boolean beginBatchEdit()
    {
        setClosing(false);
        return QtNativeInputConnection.beginBatchEdit();
    }

    @Override
    public boolean endBatchEdit()
    {
        setClosing(false);
        return QtNativeInputConnection.endBatchEdit();
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
        return QtNativeInputConnection.commitText(text.toString(), newCursorPosition);
    }

    @Override
    public boolean deleteSurroundingText(int leftLength, int rightLength)
    {
        setClosing(false);
        return QtNativeInputConnection.deleteSurroundingText(leftLength, rightLength);
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
            return QtNativeInputConnection.selectAll();
        case ID_COPY:
            return QtNativeInputConnection.copy();
        case ID_COPY_URL:
            return QtNativeInputConnection.copyURL();
        case ID_CUT:
            return QtNativeInputConnection.cut();
        case ID_PASTE:
            return QtNativeInputConnection.paste();

        case ID_SWITCH_INPUT_METHOD:
            InputMethodManager imm = (InputMethodManager)m_view.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null)
                imm.showInputMethodPicker();

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
                    break;

                default:
                   QtNative.activityDelegate().hideSoftwareKeyboard();
                   break;
            }
        }

        return super.sendKeyEvent(event);
    }

    @Override
    public boolean setComposingText(CharSequence text, int newCursorPosition)
    {
        setClosing(false);
        return QtNativeInputConnection.setComposingText(text.toString(), newCursorPosition);
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
        return QtNativeInputConnection.setSelection(start, end);
    }
}
