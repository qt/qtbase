// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.ResultReceiver;
import android.text.method.MetaKeyKeyListener;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.WindowInsets;
import android.view.WindowInsets.Type;
import android.view.WindowInsetsAnimationController;
import android.view.WindowInsetsAnimationControlListener;
import android.view.WindowManager;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.inputmethod.InputMethodManager;

class QtInputDelegate implements QtInputConnection.QtInputConnectionListener, QtInputInterface
{

    private static final String TAG = "QtInputDelegate";
    // keyboard methods
    static native void keyDown(int key, int unicode, int modifier, boolean autoRepeat);
    static native void keyUp(int key, int unicode, int modifier, boolean autoRepeat);
    static native void keyboardVisibilityChanged(boolean visibility);
    static native void keyboardGeometryChanged(int x, int y, int width, int height);
    // keyboard methods

    // dispatch events methods
    static native boolean dispatchGenericMotionEvent(MotionEvent event);
    static native boolean dispatchKeyEvent(KeyEvent event);
    // dispatch events methods

    // handle methods
    static native void handleLocationChanged(int id, int x, int y);
    // handle methods

    private QtEditText m_currentEditText = null;
    private InputMethodManager m_imm;

    // We can't rely on a hardcoded value, because screens have different resolutions.
    // That is why we assume that the keyboard should be higher than 0.15 of the screen.
    private static final float KEYBOARD_TO_SCREEN_RATIO = 0.15f;

    private boolean m_keyboardTransitionInProgress = false;
    private boolean m_keyboardIsVisible = false;
    private boolean m_isKeyboardHidingAnimationOngoing = false;
    private long m_showHideTimeStamp = System.nanoTime();
    private int m_portraitKeyboardHeight = 0;
    private int m_landscapeKeyboardHeight = 0;
    private int m_probeKeyboardHeightDelayMs = 50;

    private int m_softInputMode = 0;

    private static Boolean m_tabletEventSupported = null;

    private static int m_oldX, m_oldY;


    private long m_metaState;
    private int m_lastChar = 0;
    private boolean m_backKeyPressedSent = false;

    // Note: because of the circular call to updateFullScreen() from the delegate, we need
    // a listener to be able to do that call from the delegate, because that's where that
    // logic lives
    interface KeyboardVisibilityListener {
        void onKeyboardVisibilityChange();
    }

    private final KeyboardVisibilityListener m_keyboardVisibilityListener;

    QtInputDelegate(KeyboardVisibilityListener listener)
    {
        m_keyboardVisibilityListener = listener;
    }

    void initInputMethodManager(Activity activity)
    {
        m_imm = (InputMethodManager) activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (m_imm == null)
            Log.w(TAG, "getSystemService() returned a null InputMethodManager instance");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            View rootView = activity.getWindow().getDecorView();
            rootView.setOnApplyWindowInsetsListener(new View.OnApplyWindowInsetsListener() {
                @Override
                public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
                    if (m_keyboardIsVisible != insets.isVisible(WindowInsets.Type.ime()))
                        setKeyboardVisibility_internal(!m_keyboardIsVisible, System.nanoTime());
                    return insets;
                }
            });
        }
    }

    private final ViewTreeObserver.OnGlobalLayoutListener keyboardListener =
                                                new ViewTreeObserver.OnGlobalLayoutListener() {
        @Override
        public void onGlobalLayout() {
            if (!isKeyboardHidden())
                setKeyboardTransitionInProgress(false);
        }
    };

    private void setKeyboardTransitionInProgress(boolean state)
    {
        if (m_keyboardTransitionInProgress == state || m_currentEditText == null)
            return;

        m_keyboardTransitionInProgress = state;
        ViewTreeObserver observer = m_currentEditText.getViewTreeObserver();
        if (state)
            observer.addOnGlobalLayoutListener(keyboardListener);
        else
            observer.removeOnGlobalLayoutListener(keyboardListener);
    }

    // QtInputInterface implementation begin
    @Override
    public void updateSelection(final int selStart, final int selEnd,
                                final int candidatesStart, final int candidatesEnd)
    {
        if (m_imm != null) {
            QtNative.runAction(() -> {
                if (m_imm != null) {
                    m_imm.updateSelection(m_currentEditText, selStart, selEnd,
                            candidatesStart, candidatesEnd);
                }
            });
        }
    }

    private void showKeyboard(Activity activity,
                              final int x, final int y, final int width, final int height,
                              final int inputHints, final int enterKeyType)
    {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            activity.getWindow().getInsetsController().controlWindowInsetsAnimation(
                WindowInsets.Type.ime(), -1, null, null,
                    new WindowInsetsAnimationControlListener() {
                        @Override
                        public void onCancelled(WindowInsetsAnimationController controller) { }

                        @Override
                        public void onReady(WindowInsetsAnimationController controller, int types) { }

                        @Override
                        public void onFinished(WindowInsetsAnimationController controller) {
                            QtNativeInputConnection.updateCursorPosition();
                            if (m_softInputMode == 0)
                                probeForKeyboardHeight(activity, x, y, width, height,
                                                       inputHints, enterKeyType);
                        }
                    });
            activity.getWindow().getInsetsController().show(Type.ime());
        } else {
            if (m_imm == null)
                return;
            m_imm.showSoftInput(m_currentEditText, 0, new ResultReceiver(new Handler()) {
                @Override
                @SuppressWarnings("fallthrough")
                protected void onReceiveResult(int resultCode, Bundle resultData) {
                    switch (resultCode) {
                        case InputMethodManager.RESULT_SHOWN:
                            QtNativeInputConnection.updateCursorPosition();
                            //FALLTHROUGH
                        case InputMethodManager.RESULT_UNCHANGED_SHOWN:
                            setKeyboardVisibility(true, System.nanoTime());
                            if (m_softInputMode == 0) {
                                probeForKeyboardHeight(activity,
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
        }
    }

    @Override
    public void showSoftwareKeyboard(Activity activity,
                                     final int x, final int y, final int width, final int height,
                                     final int inputHints, final int enterKeyType)
    {
        if (m_imm == null)
            return;

        QtNative.runAction(() -> {
            if (m_imm == null || m_currentEditText == null)
                return;

            if (updateSoftInputMode(activity, height))
                return;

            m_currentEditText.setEditTextOptions(enterKeyType, inputHints);
            m_currentEditText.setLayoutParams(new QtLayout.LayoutParams(width, height, x, y));
            m_currentEditText.requestFocus();
            m_currentEditText.postDelayed(() -> {
                showKeyboard(activity, x, y, width, height, inputHints, enterKeyType);
                if (m_currentEditText.m_optionsChanged) {
                    m_imm.restartInput(m_currentEditText);
                    m_currentEditText.m_optionsChanged = false;
                }
            }, 15);
        });
    }

    @Override
    public int getSelectionHandleWidth()
    {
        return m_currentEditText == null ? 0 : m_currentEditText.getSelectionHandleWidth();
    }

    /* called from the C++ code when the position of the cursor or selection handles needs to
       be adjusted.
       mode is one of QAndroidInputContext::CursorHandleShowMode
    */
    @Override
    public void updateHandles(int mode, int editX, int editY, int editButtons,
                              int x1, int y1, int x2, int y2, boolean rtl)
    {
        QtNative.runAction(() -> {
            if (m_currentEditText != null)
                m_currentEditText.updateHandles(mode, editX, editY, editButtons, x1, y1, x2, y2, rtl);
        });
    }

    @Override
    public QtInputConnection.QtInputConnectionListener getInputConnectionListener()
    {
        return this;
    }

    @Override
    public void resetSoftwareKeyboard()
    {
        if (m_imm == null || m_currentEditText == null)
            return;
        m_currentEditText.postDelayed(() -> {
            if (m_imm == null || m_currentEditText == null)
                return;
            m_imm.restartInput(m_currentEditText);
            m_currentEditText.m_optionsChanged = false;
        }, 5);
    }

    @Override
    public void hideSoftwareKeyboard()
    {
        if (m_imm == null || m_currentEditText == null)
            return;

        m_isKeyboardHidingAnimationOngoing = true;
        QtNative.runAction(() -> {
            if (m_imm == null || m_currentEditText == null)
                return;

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                Activity activity = QtNative.activity();
                if (activity == null) {
                    Log.w(TAG, "hideSoftwareKeyboard: The activity reference is null");
                    return;
                }
                activity.getWindow().getInsetsController().hide(Type.ime());
            } else {
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
            }
        });
    }

    // Is the keyboard fully visible i.e. visible and no ongoing animation
    @Override
    public boolean isSoftwareKeyboardVisible()
    {
        return isKeyboardVisible() && !m_isKeyboardHidingAnimationOngoing;
    }
    // QtInputInterface implementation end

    // QtInputConnectionListener methods
    @Override
    public boolean keyboardTransitionInProgress() {
       return m_keyboardTransitionInProgress;
    }

    @Override
    public boolean isKeyboardHidden() {
        Activity activity = QtNative.activity();
        if (activity == null) {
            Log.w(TAG, "isKeyboardHidden: The activity reference is null");
            return true;
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R) {
            Rect r = new Rect();
            activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
            DisplayMetrics metrics = new DisplayMetrics();
            QtDisplayManager.getDisplay(activity).getMetrics(metrics);
            int screenHeight = metrics.heightPixels;
            final int kbHeight = screenHeight - r.bottom;
            return kbHeight < screenHeight * KEYBOARD_TO_SCREEN_RATIO;
        }

        return !m_keyboardIsVisible;
    }

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

    @Override
    public void onEditTextChanged(QtEditText editText) {
        setFocusedView(editText);
    }
    // QtInputConnectionListener methods

    boolean isKeyboardVisible()
    {
        return m_keyboardIsVisible;
    }

    void setSoftInputMode(int inputMode)
    {
        m_softInputMode = inputMode;
    }

    QtEditText getCurrentQtEditText()
    {
        return m_currentEditText;
    }

    private void keyboardVisibilityUpdated(boolean visibility)
    {
        m_isKeyboardHidingAnimationOngoing = false;
        QtInputDelegate.keyboardVisibilityChanged(visibility);
    }

    void setKeyboardVisibility(boolean visibility, long timeStamp)
    {
        // Since API 30 keyboard visibility changes are tracked by OnApplyWindowInsetsListener.
        // There are no manual changes anymore
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.R)
            setKeyboardVisibility_internal(visibility, timeStamp);
    }

    private void setKeyboardVisibility_internal(boolean visibility, long timeStamp)
    {
        if (m_showHideTimeStamp > timeStamp)
            return;
        m_showHideTimeStamp = timeStamp;

        if (m_keyboardIsVisible == visibility)
            return;
        m_keyboardIsVisible = visibility;
        keyboardVisibilityUpdated(m_keyboardIsVisible);
        setKeyboardTransitionInProgress(visibility);

        if (!visibility) {
            // Hiding the keyboard clears the immersive mode, so we need to set it again.
            m_keyboardVisibilityListener.onKeyboardVisibilityChange();
            if (m_currentEditText != null)
                m_currentEditText.clearFocus();
        }
    }

    void setFocusedView(QtEditText currentEditText)
    {
        setKeyboardTransitionInProgress(false);
        m_currentEditText = currentEditText;
    }

    private boolean updateSoftInputMode(Activity activity, int height)
    {
        DisplayMetrics metrics = new DisplayMetrics();
        QtDisplayManager.getDisplay(activity).getMetrics(metrics);

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

    private void probeForKeyboardHeight(Activity activity, int x, int y,
                                        int width, int height, int inputHints, int enterKeyType)
    {
        if (m_currentEditText == null) {
            Log.w(TAG, "probeForKeyboardHeight: null QtEditText");
            return;
        }
        m_currentEditText.postDelayed(() -> {
            if (!m_keyboardIsVisible)
                return;
            DisplayMetrics metrics = new DisplayMetrics();
            QtDisplayManager.getDisplay(activity).getMetrics(metrics);
            Rect r = new Rect();
            activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
            if (metrics.heightPixels != r.bottom) {
                if (metrics.widthPixels > metrics.heightPixels) { // landscape
                    if (m_landscapeKeyboardHeight != r.bottom) {
                        m_landscapeKeyboardHeight = r.bottom;
                        showSoftwareKeyboard(activity, x, y, width, height,
                                inputHints, enterKeyType);
                    }
                } else {
                    if (m_portraitKeyboardHeight != r.bottom) {
                        m_portraitKeyboardHeight = r.bottom;
                        showSoftwareKeyboard(activity, x, y, width, height,
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

    boolean onKeyDown(int keyCode, KeyEvent event)
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

    boolean onKeyUp(int keyCode, KeyEvent event)
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

    boolean handleDispatchKeyEvent(KeyEvent event)
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

    boolean handleDispatchGenericMotionEvent(MotionEvent event)
    {
        return dispatchGenericMotionEvent(event);
    }

    //////////////////////////////
    //  Mouse and Touch Input   //
    //////////////////////////////

    // tablet methods
    static native boolean isTabletEventSupported();
    static native void tabletEvent(int winId, int deviceId, long time, int action,
                                          int pointerType, int buttonState, float x, float y,
                                          float pressure);
    // tablet methods

    // pointer methods
    static native void mouseDown(int winId, int x, int y, int mouseButtonState);
    static native void mouseUp(int winId, int x, int y, int mouseButtonState);
    static native void mouseMove(int winId, int x, int y, int mouseButtonState);
    static native void mouseWheel(int winId, int x, int y, float hDelta, float vDelta);
    static native void touchBegin(int winId);
    static native void touchAdd(int winId, int pointerId, int action, boolean primary,
                                       int x, int y, float major, float minor, float rotation,
                                       float pressure);
    static native void touchEnd(int winId, int action);
    static native void touchCancel(int winId);
    static native void longPress(int winId, int x, int y);
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

    static void sendTouchEvent(MotionEvent event, int id)
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

    static void sendTrackballEvent(MotionEvent event, int id)
    {
        sendMouseEvent(event,id);
    }

    static boolean sendGenericMotionEvent(MotionEvent event, int id)
    {
        int scrollOrHoverMove = MotionEvent.ACTION_SCROLL | MotionEvent.ACTION_HOVER_MOVE;
        int pointerDeviceModifier = (event.getSource() & InputDevice.SOURCE_CLASS_POINTER);
        boolean isPointerDevice = pointerDeviceModifier == InputDevice.SOURCE_CLASS_POINTER;

        if ((event.getAction() & scrollOrHoverMove) == 0 || !isPointerDevice )
            return false;

        return sendMouseEvent(event, id);
    }

    static boolean sendMouseEvent(MotionEvent event, int id)
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
                    mouseMove(id, (int) event.getX(), (int) event.getY(), event.getButtonState());
                } else {
                    int dx = (int) (event.getX() - m_oldX);
                    int dy = (int) (event.getY() - m_oldY);
                    if (Math.abs(dx) > 5 || Math.abs(dy) > 5) {
                        mouseMove(id, (int) event.getX(), (int) event.getY(), event.getButtonState());
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
