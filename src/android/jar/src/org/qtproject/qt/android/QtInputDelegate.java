// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.os.ResultReceiver;
import android.text.method.MetaKeyKeyListener;
import android.util.DisplayMetrics;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import org.qtproject.qt.android.QtInputConnection.QtInputConnectionListener;

/** @noinspection FieldCanBeLocal*/
public class QtInputDelegate implements QtInputConnection.QtInputConnectionListener {
    // keyboard methods
    public static native void keyDown(int key, int unicode, int modifier, boolean autoRepeat);
    public static native void keyUp(int key, int unicode, int modifier, boolean autoRepeat);
    public static native void keyboardVisibilityChanged(boolean visibility);
    public static native void keyboardGeometryChanged(int x, int y, int width, int height);
    // keyboard methods

    // dispatch events methods
    public static native boolean dispatchGenericMotionEvent(MotionEvent event);
    public static native boolean dispatchKeyEvent(KeyEvent event);
    // dispatch events methods

    // handle methods
    public static native void handleLocationChanged(int id, int x, int y);
    // handle methods

    private QtEditText m_currentEditText = null;
    private final InputMethodManager m_imm;

    private boolean m_keyboardIsVisible = false;
    private boolean m_isKeyboardHidingAnimationOngoing = false;
    private long m_showHideTimeStamp = System.nanoTime();
    private int m_portraitKeyboardHeight = 0;
    private int m_landscapeKeyboardHeight = 0;
    private int m_probeKeyboardHeightDelayMs = 50;
    private CursorHandle m_cursorHandle;
    private CursorHandle m_leftSelectionHandle;
    private CursorHandle m_rightSelectionHandle;
    private EditPopupMenu m_editPopupMenu;

    private int m_softInputMode = 0;

    // Values coming from QAndroidInputContext::CursorHandleShowMode
    private static final int CursorHandleNotShown       = 0;
    private static final int CursorHandleShowNormal     = 1;
    private static final int CursorHandleShowSelection  = 2;
    private static final int CursorHandleShowEdit       = 0x100;

    // Handle IDs
    public static final int IdCursorHandle = 1;
    public static final int IdLeftHandle = 2;
    public static final int IdRightHandle = 3;

    private static Boolean m_tabletEventSupported = null;

    private static int m_oldX, m_oldY;


    private long m_metaState;
    private int m_lastChar = 0;
    private boolean m_backKeyPressedSent = false;

    // Note: because of the circular call to updateFullScreen() from the delegate, we need
    // a listener to be able to do that call from the delegate, because that's where that
    // logic lives
    public interface KeyboardVisibilityListener {
        void onKeyboardVisibilityChange();
    }

    private final KeyboardVisibilityListener m_keyboardVisibilityListener;

    QtInputDelegate(Activity activity, KeyboardVisibilityListener listener)
    {
        this.m_keyboardVisibilityListener = listener;
        m_imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
    }

    // QtInputConnectionListener methods
    @Override
    public void onSetClosing(boolean closing) {
        if (!closing)
            setKeyboardVisibility(true, System.nanoTime());
    }

    @Override
    public void onHideKeyboardRunnableDone(boolean visibility, long hideTimeStamp) {
        setKeyboardVisibility(visibility, hideTimeStamp);
    }

    @Override
    public void onSendKeyEventDefaultCase() {
        hideSoftwareKeyboard();
    }
    // QtInputConnectionListener methods

    public boolean isKeyboardVisible()
    {
        return m_keyboardIsVisible;
    }

    // Is the keyboard fully visible i.e. visible and no ongoing animation
    @UsedFromNativeCode
    public boolean isSoftwareKeyboardVisible()
    {
        return isKeyboardVisible() && !m_isKeyboardHidingAnimationOngoing;
    }

    void setSoftInputMode(int inputMode)
    {
        m_softInputMode = inputMode;
    }

    QtEditText getCurrentQtEditText()
    {
        return m_currentEditText;
    }

    void setEditPopupMenu(EditPopupMenu editPopupMenu)
    {
        m_editPopupMenu = editPopupMenu;
    }

    private void keyboardVisibilityUpdated(boolean visibility)
    {
        m_isKeyboardHidingAnimationOngoing = false;
        QtInputDelegate.keyboardVisibilityChanged(visibility);
    }

    public void setKeyboardVisibility(boolean visibility, long timeStamp)
    {
        if (m_showHideTimeStamp > timeStamp)
            return;
        m_showHideTimeStamp = timeStamp;

        if (m_keyboardIsVisible == visibility)
            return;
        m_keyboardIsVisible = visibility;
        keyboardVisibilityUpdated(m_keyboardIsVisible);
        if (!visibility) {
            // Hiding the keyboard clears the immersive mode, so we need to set it again.
            m_keyboardVisibilityListener.onKeyboardVisibilityChange();
            m_currentEditText.clearFocus();
        }
    }

    @UsedFromNativeCode
    public void resetSoftwareKeyboard()
    {
        if (m_imm == null || m_currentEditText == null)
            return;
        m_currentEditText.postDelayed(() -> {
            m_imm.restartInput(m_currentEditText);
            m_currentEditText.m_optionsChanged = false;
        }, 5);
    }

    void setFocusedView(QtEditText currentEditText)
    {
        m_currentEditText = currentEditText;
    }

    public void showSoftwareKeyboard(Activity activity, QtLayout layout,
                                     final int x, final int y, final int width, final int height,
                                     final int inputHints, final int enterKeyType)
    {
        QtNative.runAction(() -> {
            if (m_imm == null || m_currentEditText == null)
                return;

            if (updateSoftInputMode(activity, height))
                return;

            m_currentEditText.setEditTextOptions(enterKeyType, inputHints);
            m_currentEditText.requestFocus();

            m_currentEditText.postDelayed(() -> {
                m_imm.showSoftInput(m_currentEditText, 0, new ResultReceiver(new Handler()) {
                    @Override
                    protected void onReceiveResult(int resultCode, Bundle resultData) {
                        switch (resultCode) {
                            case InputMethodManager.RESULT_SHOWN:
                                QtNativeInputConnection.updateCursorPosition();
                                //FALLTHROUGH
                            case InputMethodManager.RESULT_UNCHANGED_SHOWN:
                                setKeyboardVisibility(true, System.nanoTime());
                                if (m_softInputMode == 0) {
                                    probeForKeyboardHeight(layout, activity,
                                            x, y, width, height, inputHints, enterKeyType);
                                }
                                break;
                            case InputMethodManager.RESULT_HIDDEN:
                            case InputMethodManager.RESULT_UNCHANGED_HIDDEN:
                                setKeyboardVisibility(false, System.nanoTime());
                                break;
                        }
                    }
                });
                if (m_currentEditText.m_optionsChanged) {
                    m_imm.restartInput(m_currentEditText);
                    m_currentEditText.m_optionsChanged = false;
                }
            }, 15);
        });
    }

    private boolean updateSoftInputMode(Activity activity, int height)
    {
        DisplayMetrics metrics = new DisplayMetrics();
        activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        // If the screen is in portrait mode than we estimate that keyboard height
        // will not be higher than 2/5 of the screen. Otherwise we estimate that keyboard height
        // will not be higher than 2/3 of the screen
        final int visibleHeight;
        if (metrics.widthPixels < metrics.heightPixels) {
            visibleHeight = m_portraitKeyboardHeight != 0 ?
                    m_portraitKeyboardHeight : metrics.heightPixels * 3 / 5;
        } else {
            visibleHeight = m_landscapeKeyboardHeight != 0 ?
                    m_landscapeKeyboardHeight : metrics.heightPixels / 3;
        }

        if (m_softInputMode != 0) {
            activity.getWindow().setSoftInputMode(m_softInputMode);
            int stateHidden = WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN;
            return (m_softInputMode & stateHidden) != 0;
        } else {
            int stateUnchanged = WindowManager.LayoutParams.SOFT_INPUT_STATE_UNCHANGED;
            if (height > visibleHeight) {
                int adjustResize = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE;
                activity.getWindow().setSoftInputMode(stateUnchanged | adjustResize);
            } else {
                int adjustPan = WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
                activity.getWindow().setSoftInputMode(stateUnchanged | adjustPan);
            }
        }
        return false;
    }

    private void probeForKeyboardHeight(QtLayout layout, Activity activity, int x, int y,
                                        int width, int height, int inputHints, int enterKeyType)
    {
        layout.postDelayed(() -> {
            if (!m_keyboardIsVisible)
                return;
            DisplayMetrics metrics = new DisplayMetrics();
            activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
            Rect r = new Rect();
            activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
            if (metrics.heightPixels != r.bottom) {
                if (metrics.widthPixels > metrics.heightPixels) { // landscape
                    if (m_landscapeKeyboardHeight != r.bottom) {
                        m_landscapeKeyboardHeight = r.bottom;
                        showSoftwareKeyboard(activity, layout, x, y, width, height,
                                inputHints, enterKeyType);
                    }
                } else {
                    if (m_portraitKeyboardHeight != r.bottom) {
                        m_portraitKeyboardHeight = r.bottom;
                        showSoftwareKeyboard(activity, layout, x, y, width, height,
                                inputHints, enterKeyType);
                    }
                }
            } else {
                // no luck ?
                // maybe the delay was too short, so let's make it longer
                if (m_probeKeyboardHeightDelayMs < 1000)
                    m_probeKeyboardHeightDelayMs *= 2;
            }
        }, m_probeKeyboardHeightDelayMs);
    }

    public void hideSoftwareKeyboard()
    {
        m_isKeyboardHidingAnimationOngoing = true;
        QtNative.runAction(() -> {
            if (m_imm == null || m_currentEditText == null)
                return;

            m_imm.hideSoftInputFromWindow(m_currentEditText.getWindowToken(), 0,
                    new ResultReceiver(new Handler()) {
                @Override
                protected void onReceiveResult(int resultCode, Bundle resultData) {
                    switch (resultCode) {
                        case InputMethodManager.RESULT_SHOWN:
                        case InputMethodManager.RESULT_UNCHANGED_SHOWN:
                            setKeyboardVisibility(true, System.nanoTime());
                            break;
                        case InputMethodManager.RESULT_HIDDEN:
                        case InputMethodManager.RESULT_UNCHANGED_HIDDEN:
                            setKeyboardVisibility(false, System.nanoTime());
                            break;
                    }
                }
            });
        });
    }

    @UsedFromNativeCode
    public void updateSelection(final int selStart, final int selEnd,
                                final int candidatesStart, final int candidatesEnd)
    {
        QtNative.runAction(() -> {
            if (m_imm == null)
                return;

            m_imm.updateSelection(m_currentEditText, selStart, selEnd, candidatesStart, candidatesEnd);
        });
    }

    @UsedFromNativeCode
    public int getSelectHandleWidth()
    {
        int width = 0;
        if (m_leftSelectionHandle != null && m_rightSelectionHandle != null) {
            width = Math.max(m_leftSelectionHandle.width(), m_rightSelectionHandle.width());
        } else if (m_cursorHandle != null) {
            width = m_cursorHandle.width();
        }
        return width;
    }

    /* called from the C++ code when the position of the cursor or selection handles needs to
       be adjusted.
       mode is one of QAndroidInputContext::CursorHandleShowMode
    */
    @UsedFromNativeCode
    public void updateHandles(Activity activity, QtLayout layout, int mode,
                              int editX, int editY, int editButtons,
                              int x1, int y1, int x2, int y2, boolean rtl)
    {
        QtNative.runAction(() -> updateHandleImpl(activity, layout, mode, editX, editY, editButtons,
                x1, y1, x2, y2, rtl));
    }

    private void updateHandleImpl(Activity activity, QtLayout layout, int mode,
                               int editX, int editY, int editButtons,
                               int x1, int y1, int x2, int y2, boolean rtl)
    {
        switch (mode & 0xff)
        {
            case CursorHandleNotShown:
                if (m_cursorHandle != null) {
                    m_cursorHandle.hide();
                    m_cursorHandle = null;
                }
                if (m_rightSelectionHandle != null) {
                    m_rightSelectionHandle.hide();
                    m_leftSelectionHandle.hide();
                    m_rightSelectionHandle = null;
                    m_leftSelectionHandle = null;
                }
                if (m_editPopupMenu != null)
                    m_editPopupMenu.hide();
                break;

            case CursorHandleShowNormal:
                if (m_cursorHandle == null) {
                    m_cursorHandle = new CursorHandle(activity, layout, IdCursorHandle,
                            android.R.attr.textSelectHandle, false);
                }
                m_cursorHandle.setPosition(x1, y1);
                if (m_rightSelectionHandle != null) {
                    m_rightSelectionHandle.hide();
                    m_leftSelectionHandle.hide();
                    m_rightSelectionHandle = null;
                    m_leftSelectionHandle = null;
                }
                break;

            case CursorHandleShowSelection:
                if (m_rightSelectionHandle == null) {
                    m_leftSelectionHandle = new CursorHandle(activity, layout, IdLeftHandle,
                            !rtl ? android.R.attr.textSelectHandleLeft :
                                    android.R.attr.textSelectHandleRight,
                            rtl);
                    m_rightSelectionHandle = new CursorHandle(activity, layout, IdRightHandle,
                            !rtl ? android.R.attr.textSelectHandleRight :
                                    android.R.attr.textSelectHandleLeft,
                            rtl);
                }
                m_leftSelectionHandle.setPosition(x1,y1);
                m_rightSelectionHandle.setPosition(x2,y2);
                if (m_cursorHandle != null) {
                    m_cursorHandle.hide();
                    m_cursorHandle = null;
                }
                mode |= CursorHandleShowEdit;
                break;
        }

        if (!QtClipboardManager.hasClipboardText(activity))
            editButtons &= ~EditContextView.PASTE_BUTTON;

        if (m_editPopupMenu != null) {
            if ((mode & CursorHandleShowEdit) == CursorHandleShowEdit && editButtons != 0) {
                m_editPopupMenu.setPosition(editX, editY, editButtons,
                        m_cursorHandle, m_leftSelectionHandle, m_rightSelectionHandle);
            } else {
                m_editPopupMenu.hide();
            }
        }
    }

    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        m_metaState = MetaKeyKeyListener.handleKeyDown(m_metaState, keyCode, event);
        int metaState = MetaKeyKeyListener.getMetaState(m_metaState) | event.getMetaState();
        int c = event.getUnicodeChar(metaState);
        int lc = c;
        m_metaState = MetaKeyKeyListener.adjustMetaAfterKeypress(m_metaState);

        if ((c & KeyCharacterMap.COMBINING_ACCENT) != 0) {
            c = c & KeyCharacterMap.COMBINING_ACCENT_MASK;
            c = KeyEvent.getDeadChar(m_lastChar, c);
        }

        if ((keyCode == KeyEvent.KEYCODE_VOLUME_UP
                || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
                || keyCode == KeyEvent.KEYCODE_MUTE)
                && System.getenv("QT_ANDROID_VOLUME_KEYS") == null) {
            return false;
        }

        m_lastChar = lc;
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            m_backKeyPressedSent = !isKeyboardVisible();
            if (!m_backKeyPressedSent)
                return true;
        }

        QtInputDelegate.keyDown(keyCode, c, event.getMetaState(), event.getRepeatCount() > 0);

        return true;
    }

    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if ((keyCode == KeyEvent.KEYCODE_VOLUME_UP
                || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN
                || keyCode == KeyEvent.KEYCODE_MUTE)
                && System.getenv("QT_ANDROID_VOLUME_KEYS") == null) {
            return false;
        }

        if (keyCode == KeyEvent.KEYCODE_BACK && !m_backKeyPressedSent) {
            hideSoftwareKeyboard();
            setKeyboardVisibility(false, System.nanoTime());
            return true;
        }

        m_metaState = MetaKeyKeyListener.handleKeyUp(m_metaState, keyCode, event);
        boolean autoRepeat = event.getRepeatCount() > 0;
        QtInputDelegate.keyUp(keyCode, event.getUnicodeChar(), event.getMetaState(), autoRepeat);

        return true;
    }

    public boolean handleDispatchKeyEvent(KeyEvent event)
    {
        if (event.getAction() == KeyEvent.ACTION_MULTIPLE
                && event.getCharacters() != null
                && event.getCharacters().length() == 1
                && event.getKeyCode() == 0) {
            keyDown(0, event.getCharacters().charAt(0), event.getMetaState(),
                    event.getRepeatCount() > 0);
            keyUp(0, event.getCharacters().charAt(0), event.getMetaState(),
                    event.getRepeatCount() > 0);
        }

        return dispatchKeyEvent(event);
    }

    public boolean handleDispatchGenericMotionEvent(MotionEvent event)
    {
        return dispatchGenericMotionEvent(event);
    }

    //////////////////////////////
    //  Mouse and Touch Input   //
    //////////////////////////////

    // tablet methods
    public static native boolean isTabletEventSupported();
    public static native void tabletEvent(int winId, int deviceId, long time, int action,
                                          int pointerType, int buttonState, float x, float y,
                                          float pressure);
    // tablet methods

    // pointer methods
    public static native void mouseDown(int winId, int x, int y, int mouseButtonState);
    public static native void mouseUp(int winId, int x, int y, int mouseButtonState);
    public static native void mouseMove(int winId, int x, int y);
    public static native void mouseWheel(int winId, int x, int y, float hDelta, float vDelta);
    public static native void touchBegin(int winId);
    public static native void touchAdd(int winId, int pointerId, int action, boolean primary,
                                       int x, int y, float major, float minor, float rotation,
                                       float pressure);
    public static native void touchEnd(int winId, int action);
    public static native void touchCancel(int winId);
    public static native void longPress(int winId, int x, int y);
    // pointer methods

    static private int getAction(int index, MotionEvent event)
    {
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE) {
            int hsz = event.getHistorySize();
            if (hsz > 0) {
                float x = event.getX(index);
                float y = event.getY(index);
                for (int h = 0; h < hsz; ++h) {
                    if ( event.getHistoricalX(index, h) != x ||
                            event.getHistoricalY(index, h) != y )
                        return 1;
                }
                return 2;
            }
            return 1;
        }
        if (action == MotionEvent.ACTION_DOWN
                || action == MotionEvent.ACTION_POINTER_DOWN && index == event.getActionIndex()) {
            return 0;
        } else if (action == MotionEvent.ACTION_UP
                || action == MotionEvent.ACTION_POINTER_UP && index == event.getActionIndex()) {
            return 3;
        }
        return 2;
    }

    static public void sendTouchEvent(MotionEvent event, int id)
    {
        int pointerType = 0;

        if (m_tabletEventSupported == null)
            m_tabletEventSupported = isTabletEventSupported();

        switch (event.getToolType(0)) {
            case MotionEvent.TOOL_TYPE_STYLUS:
                pointerType = 1; // QTabletEvent::Pen
                break;
            case MotionEvent.TOOL_TYPE_ERASER:
                pointerType = 3; // QTabletEvent::Eraser
                break;
        }

        if (event.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE) {
            sendMouseEvent(event, id);
        } else if (m_tabletEventSupported && pointerType != 0) {
            tabletEvent(id, event.getDeviceId(), event.getEventTime(), event.getActionMasked(),
                    pointerType, event.getButtonState(),
                    event.getX(), event.getY(), event.getPressure());
        } else {
            touchBegin(id);
            for (int i = 0; i < event.getPointerCount(); ++i) {
                touchAdd(id,
                        event.getPointerId(i),
                        getAction(i, event),
                        i == 0,
                        (int)event.getX(i),
                        (int)event.getY(i),
                        event.getTouchMajor(i),
                        event.getTouchMinor(i),
                        event.getOrientation(i),
                        event.getPressure(i));
            }

            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    touchEnd(id, 0);
                    break;

                case MotionEvent.ACTION_UP:
                    touchEnd(id, 2);
                    break;

                case MotionEvent.ACTION_CANCEL:
                    touchCancel(id);
                    break;

                default:
                    touchEnd(id, 1);
            }
        }
    }

    static public void sendTrackballEvent(MotionEvent event, int id)
    {
        sendMouseEvent(event,id);
    }

    static public boolean sendGenericMotionEvent(MotionEvent event, int id)
    {
        int scrollOrHoverMove = MotionEvent.ACTION_SCROLL | MotionEvent.ACTION_HOVER_MOVE;
        int pointerDeviceModifier = (event.getSource() & InputDevice.SOURCE_CLASS_POINTER);
        boolean isPointerDevice = pointerDeviceModifier == InputDevice.SOURCE_CLASS_POINTER;

        if ((event.getAction() & scrollOrHoverMove) == 0 || !isPointerDevice )
            return false;

        return sendMouseEvent(event, id);
    }

    static public boolean sendMouseEvent(MotionEvent event, int id)
    {
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_UP:
                mouseUp(id, (int) event.getX(), (int) event.getY(), event.getButtonState());
                break;

            case MotionEvent.ACTION_DOWN:
                mouseDown(id, (int) event.getX(), (int) event.getY(), event.getButtonState());
                m_oldX = (int) event.getX();
                m_oldY = (int) event.getY();
                break;
            case MotionEvent.ACTION_HOVER_MOVE:
            case MotionEvent.ACTION_MOVE:
                if (event.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE) {
                    mouseMove(id, (int) event.getX(), (int) event.getY());
                } else {
                    int dx = (int) (event.getX() - m_oldX);
                    int dy = (int) (event.getY() - m_oldY);
                    if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                        mouseMove(id, (int) event.getX(), (int) event.getY());
                        m_oldX = (int) event.getX();
                        m_oldY = (int) event.getY();
                    }
                }
                break;
            case MotionEvent.ACTION_SCROLL:
                mouseWheel(id, (int) event.getX(), (int) event.getY(),
                        event.getAxisValue(MotionEvent.AXIS_HSCROLL),
                        event.getAxisValue(MotionEvent.AXIS_VSCROLL));
                break;
            default:
                return false;
        }
        return true;
    }
}
