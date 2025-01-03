// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


package org.qtproject.qt.android.accessibility;

import android.accessibilityservice.AccessibilityService;
import android.app.Activity;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.text.TextUtils;

import android.view.accessibility.*;
import android.view.accessibility.AccessibilityNodeInfo.CollectionInfo;
import android.view.MotionEvent;
import android.view.View.OnHoverListener;

import android.content.Context;
import android.system.Os;

import java.util.LinkedList;
import java.util.List;

import org.qtproject.qt.android.QtActivityDelegate;

public class QtAccessibilityDelegate extends View.AccessibilityDelegate
{
    private static final String TAG = "Qt A11Y";

    // Qt uses the upper half of the unsiged integers
    // all low positive ints should be fine.
    public static final int INVALID_ID = 333; // half evil

    // The platform might ask for the class implementing the "view".
    // Pretend to be an inner class of the QtSurface.
    private static final String DEFAULT_CLASS_NAME = "$VirtualChild";

    private View m_view = null;
    private AccessibilityManager m_manager;
    private QtActivityDelegate m_activityDelegate;
    private Activity m_activity;
    private ViewGroup m_layout;

    // The accessible object that currently has the "accessibility focus"
    // usually indicated by a yellow rectangle on screen.
    private int m_focusedVirtualViewId = INVALID_ID;
    // When exploring the screen by touch, the item "hovered" by the finger.
    private int m_hoveredVirtualViewId = INVALID_ID;

    // Cache coordinates of the view to know the global offset
    // this is because the Android platform window does not take
    // the offset of the view on screen into account (eg status bar on top)
    private final int[] m_globalOffset = new int[2];
    private int m_oldOffsetX = 0;
    private int m_oldOffsetY = 0;

    private class HoverEventListener implements View.OnHoverListener
    {
        @Override
        public boolean onHover(View v, MotionEvent event)
        {
            return dispatchHoverEvent(event);
        }
    }

    public QtAccessibilityDelegate(Activity activity, ViewGroup layout, QtActivityDelegate activityDelegate)
    {
        m_activity = activity;
        m_layout = layout;
        m_activityDelegate = activityDelegate;

        m_manager = (AccessibilityManager) m_activity.getSystemService(Context.ACCESSIBILITY_SERVICE);
        if (m_manager != null) {
            AccessibilityManagerListener accServiceListener = new AccessibilityManagerListener();
            if (!m_manager.addAccessibilityStateChangeListener(accServiceListener))
                Log.w("Qt A11y", "Could not register a11y state change listener");
            if (m_manager.isEnabled())
                accServiceListener.onAccessibilityStateChanged(true);
        }
    }

    private class AccessibilityManagerListener implements AccessibilityManager.AccessibilityStateChangeListener
    {
        @Override
        public void onAccessibilityStateChanged(boolean enabled)
        {
            if (Os.getenv("QT_ANDROID_DISABLE_ACCESSIBILITY") != null)
                return;
            if (enabled) {
                    try {
                        View view = m_view;
                        if (view == null) {
                            view = new View(m_activity);
                            view.setId(View.NO_ID);
                        }

                        // ### Keep this for debugging for a while. It allows us to visually see that our View
                        // ### is on top of the surface(s)
                        // ColorDrawable color = new ColorDrawable(0x80ff8080);    //0xAARRGGBB
                        // view.setBackground(color);
                        view.setAccessibilityDelegate(QtAccessibilityDelegate.this);

                        // if all is fine, add it to the layout
                        if (m_view == null) {
                            //m_layout.addAccessibilityView(view);
                            m_layout.addView(view, m_activityDelegate.getSurfaceCount(),
                                             new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                        }
                        m_view = view;

                        m_view.setOnHoverListener(new HoverEventListener());
                    } catch (Exception e) {
                        // Unknown exception means something went wrong.
                        Log.w("Qt A11y", "Unknown exception: " + e.toString());
                    }
            } else {
                if (m_view != null) {
                    m_layout.removeView(m_view);
                    m_view = null;
                }
            }

            QtNativeAccessibility.setActive(enabled);
        }
    }


    @Override
    public AccessibilityNodeProvider getAccessibilityNodeProvider(View host)
    {
        return m_nodeProvider;
    }

    // For "explore by touch" we need all movement events here first
    // (user moves finger over screen to discover items on screen).
    private boolean dispatchHoverEvent(MotionEvent event)
    {
        if (!m_manager.isTouchExplorationEnabled()) {
            return false;
        }

        int virtualViewId = QtNativeAccessibility.hitTest(event.getX(), event.getY());
        if (virtualViewId == INVALID_ID) {
            virtualViewId = View.NO_ID;
        }

        switch (event.getAction()) {
            case MotionEvent.ACTION_HOVER_ENTER:
            case MotionEvent.ACTION_HOVER_MOVE:
                setHoveredVirtualViewId(virtualViewId);
                break;
            case MotionEvent.ACTION_HOVER_EXIT:
                setHoveredVirtualViewId(virtualViewId);
                break;
        }

        return true;
    }

    public void notifyScrolledEvent(int viewId)
    {
        sendEventForVirtualViewId(viewId, AccessibilityEvent.TYPE_VIEW_SCROLLED);
    }

    public void notifyLocationChange(int viewId)
    {
        if (m_focusedVirtualViewId == viewId)
            invalidateVirtualViewId(m_focusedVirtualViewId);
    }

    public void notifyObjectHide(int viewId, int parentId)
    {
        // If the object had accessibility focus, we need to clear it.
        // Note: This code is mostly copied from
        // AccessibilityNodeProvider::performAction, but we remove the
        // focus only if the focused view id matches the one that was hidden.
        if (m_focusedVirtualViewId == viewId) {
            m_focusedVirtualViewId = INVALID_ID;
            m_view.invalidate();
            sendEventForVirtualViewId(viewId,
                    AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED);
        }
        // When the object is hidden, we need to notify its parent about
        // content change, not the hidden object itself
        invalidateVirtualViewId(parentId);
    }

    public void notifyObjectFocus(int viewId)
    {
        if (m_view == null)
            return;
        m_focusedVirtualViewId = viewId;
        m_view.invalidate();
        sendEventForVirtualViewId(viewId,
                AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED);
    }

    public void notifyValueChanged(int viewId, String value)
    {
        // Send a TYPE_ANNOUNCEMENT event with the new value

        if ((viewId == INVALID_ID) || !m_manager.isEnabled()) {
            Log.w(TAG, "notifyValueChanged() for invalid view");
            return;
        }

        final ViewGroup group = (ViewGroup)m_view.getParent();
        if (group == null) {
            Log.w(TAG, "Could not announce value because ViewGroup was null.");
            return;
        }

        final AccessibilityEvent event =
                AccessibilityEvent.obtain(AccessibilityEvent.TYPE_ANNOUNCEMENT);

        event.setEnabled(true);
        event.setClassName(m_view.getClass().getName() + DEFAULT_CLASS_NAME);

        event.setContentDescription(value);

        if (event.getText().isEmpty() && TextUtils.isEmpty(event.getContentDescription())) {
            Log.w(TAG, "No value to announce for " + event.getClassName());
            return;
        }

        event.setPackageName(m_view.getContext().getPackageName());
        event.setSource(m_view, viewId);

        if (!group.requestSendAccessibilityEvent(m_view, event))
            Log.w(TAG, "Failed to send value change announcement for " + event.getClassName());
    }

    public boolean sendEventForVirtualViewId(int virtualViewId, int eventType)
    {
        final AccessibilityEvent event = getEventForVirtualViewId(virtualViewId, eventType);
        return sendAccessibilityEvent(event);
    }

    public boolean sendAccessibilityEvent(AccessibilityEvent event)
    {
        if (event == null)
            return false;

        final ViewGroup group = (ViewGroup) m_view.getParent();
        if (group == null) {
            Log.w(TAG, "Could not send AccessibilityEvent because group was null. This should really not happen.");
            return false;
        }

        return group.requestSendAccessibilityEvent(m_view, event);
    }

    public void invalidateVirtualViewId(int virtualViewId)
    {
        final AccessibilityEvent event = getEventForVirtualViewId(virtualViewId, AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED);

        if (event == null)
            return;

        event.setContentChangeTypes(AccessibilityEvent.CONTENT_CHANGE_TYPE_SUBTREE);
        sendAccessibilityEvent(event);
    }

    private void setHoveredVirtualViewId(int virtualViewId)
    {
        if (m_hoveredVirtualViewId == virtualViewId) {
            return;
        }

        final int previousVirtualViewId = m_hoveredVirtualViewId;
        m_hoveredVirtualViewId = virtualViewId;
        sendEventForVirtualViewId(virtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_ENTER);
        sendEventForVirtualViewId(previousVirtualViewId, AccessibilityEvent.TYPE_VIEW_HOVER_EXIT);
    }

    private AccessibilityEvent getEventForVirtualViewId(int virtualViewId, int eventType)
    {
        if ((virtualViewId == INVALID_ID) || !m_manager.isEnabled()) {
            Log.w(TAG, "getEventForVirtualViewId for invalid view");
            return null;
        }

        if (m_activityDelegate.getSurfaceCount() == 0)
            return null;

        final AccessibilityEvent event = AccessibilityEvent.obtain(eventType);

        event.setEnabled(true);
        event.setClassName(m_view.getClass().getName() + DEFAULT_CLASS_NAME);

        event.setContentDescription(QtNativeAccessibility.descriptionForAccessibleObject(virtualViewId));
        if (event.getText().isEmpty() && TextUtils.isEmpty(event.getContentDescription()))
            Log.w(TAG, "AccessibilityEvent with empty description");

        event.setPackageName(m_view.getContext().getPackageName());
        event.setSource(m_view, virtualViewId);
        return event;
    }

    private void dumpNodes(int parentId)
    {
        Log.i(TAG, "A11Y hierarchy: " + parentId + " parent: " + QtNativeAccessibility.parentId(parentId));
        Log.i(TAG, "    desc: " + QtNativeAccessibility.descriptionForAccessibleObject(parentId) + " rect: " + QtNativeAccessibility.screenRect(parentId));
        Log.i(TAG, " NODE: " + getNodeForVirtualViewId(parentId));
        int[] ids = QtNativeAccessibility.childIdListForAccessibleObject(parentId);
        for (int i = 0; i < ids.length; ++i) {
            Log.i(TAG, parentId + " has child: " + ids[i]);
            dumpNodes(ids[i]);
        }
    }

    private AccessibilityNodeInfo getNodeForView()
    {
        // Since we don't want the parent to be focusable, but we can't remove
        // actions from a node, copy over the necessary fields.
        final AccessibilityNodeInfo result = AccessibilityNodeInfo.obtain(m_view);
        final AccessibilityNodeInfo source = AccessibilityNodeInfo.obtain(m_view);
        m_view.onInitializeAccessibilityNodeInfo(source);

        // Get the actual position on screen, taking the status bar into account.
        m_view.getLocationOnScreen(m_globalOffset);
        final int offsetX = m_globalOffset[0];
        final int offsetY = m_globalOffset[1];

        // Copy over parent and screen bounds.
        final Rect m_tempParentRect = new Rect();
        source.getBoundsInParent(m_tempParentRect);
        result.setBoundsInParent(m_tempParentRect);

        final Rect m_tempScreenRect = new Rect();
        source.getBoundsInScreen(m_tempScreenRect);
        m_tempScreenRect.offset(offsetX, offsetY);
        result.setBoundsInScreen(m_tempScreenRect);

        // Set up the parent view, if applicable.
        final ViewParent parent = m_view.getParent();
        if (parent instanceof View) {
            result.setParent((View) parent);
        }

        result.setVisibleToUser(source.isVisibleToUser());
        result.setPackageName(source.getPackageName());
        result.setClassName(source.getClassName());

// Spit out the entire hierarchy for debugging purposes
//        dumpNodes(-1);

        if (m_activityDelegate.getSurfaceCount() != 0) {
            int[] ids = QtNativeAccessibility.childIdListForAccessibleObject(-1);
            for (int i = 0; i < ids.length; ++i)
                result.addChild(m_view, ids[i]);
        }

        // The offset values have changed, so we need to re-focus the
        // currently focused item, otherwise it will have an incorrect
        // focus frame
        if ((m_oldOffsetX != offsetX) || (m_oldOffsetY != offsetY)) {
            m_oldOffsetX = offsetX;
            m_oldOffsetY = offsetY;
            if (m_focusedVirtualViewId != INVALID_ID) {
                m_nodeProvider.performAction(m_focusedVirtualViewId,
                                             AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS,
                                             new Bundle());
                m_nodeProvider.performAction(m_focusedVirtualViewId,
                                             AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS,
                                             new Bundle());
            }
        }

        return result;
    }

    private AccessibilityNodeInfo getNodeForVirtualViewId(int virtualViewId)
    {
        final AccessibilityNodeInfo node = AccessibilityNodeInfo.obtain();

        node.setClassName(m_view.getClass().getName() + DEFAULT_CLASS_NAME);
        node.setPackageName(m_view.getContext().getPackageName());

        if (m_activityDelegate.getSurfaceCount() == 0 || !QtNativeAccessibility.populateNode(virtualViewId, node)) {
            return node;
        }

        // set only if valid, otherwise we return a node that is invalid and will crash when accessed
        node.setSource(m_view, virtualViewId);

        if (TextUtils.isEmpty(node.getText()) && TextUtils.isEmpty(node.getContentDescription()))
            Log.w(TAG, "AccessibilityNodeInfo with empty contentDescription: " + virtualViewId);

        int parentId = QtNativeAccessibility.parentId(virtualViewId);
        node.setParent(m_view, parentId);

        Rect screenRect = QtNativeAccessibility.screenRect(virtualViewId);
        final int offsetX = m_globalOffset[0];
        final int offsetY = m_globalOffset[1];
        screenRect.offset(offsetX, offsetY);
        node.setBoundsInScreen(screenRect);

        Rect rectInParent = screenRect;
        Rect parentScreenRect = QtNativeAccessibility.screenRect(parentId);
        rectInParent.offset(-parentScreenRect.left, -parentScreenRect.top);
        node.setBoundsInParent(rectInParent);

        // Manage internal accessibility focus state.
        if (m_focusedVirtualViewId == virtualViewId) {
            node.setAccessibilityFocused(true);
            node.addAction(AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS);
        } else {
            node.setAccessibilityFocused(false);
            node.addAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
        }

        int[] ids = QtNativeAccessibility.childIdListForAccessibleObject(virtualViewId);
        for (int i = 0; i < ids.length; ++i)
            node.addChild(m_view, ids[i]);
        if (node.isScrollable()) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
                node.setCollectionInfo(new CollectionInfo(ids.length, 1, false));
            } else {
                node.setCollectionInfo(CollectionInfo.obtain(ids.length, 1, false));
            }
        }

        return node;
    }

    private AccessibilityNodeProvider m_nodeProvider = new AccessibilityNodeProvider()
    {
        @Override
        public AccessibilityNodeInfo createAccessibilityNodeInfo(int virtualViewId)
        {
            if (virtualViewId == View.NO_ID || m_activityDelegate.getSurfaceCount() == 0) {
                return getNodeForView();
            }
            return getNodeForVirtualViewId(virtualViewId);
        }

        @Override
        public boolean performAction(int virtualViewId, int action, Bundle arguments)
        {
            boolean handled = false;
            //Log.i(TAG, "PERFORM ACTION: " + action + " on " + virtualViewId);
            switch (action) {
                case AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS:
                    // Only handle the FOCUS action if it's placing focus on
                    // a different view that was previously focused.
                    if (m_focusedVirtualViewId != virtualViewId) {
                        m_focusedVirtualViewId = virtualViewId;
                        m_view.invalidate();
                        sendEventForVirtualViewId(virtualViewId,
                                AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED);
                        handled = true;
                    }
                    break;
                case AccessibilityNodeInfo.ACTION_CLEAR_ACCESSIBILITY_FOCUS:
                    if (m_focusedVirtualViewId == virtualViewId) {
                        m_focusedVirtualViewId = INVALID_ID;
                    }
                    // Since we're managing focus at the parent level, we are
                    // likely to receive a FOCUS action before a CLEAR_FOCUS
                    // action. We'll give the benefit of the doubt to the
                    // framework and always handle FOCUS_CLEARED.
                    m_view.invalidate();
                    sendEventForVirtualViewId(virtualViewId,
                            AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED);
                    handled = true;
                    break;
                default:
                    // Let the node provider handle focus for the view node.
                    if (virtualViewId == View.NO_ID) {
                        return m_view.performAccessibilityAction(action, arguments);
                    }
            }
            handled |= performActionForVirtualViewId(virtualViewId, action, arguments);

            return handled;
        }
    };

    protected boolean performActionForVirtualViewId(int virtualViewId, int action, Bundle arguments)
    {
//        Log.i(TAG, "ACTION " + action + " on " + virtualViewId);
//        dumpNodes(virtualViewId);
        boolean success = false;
        switch (action) {
        case AccessibilityNodeInfo.ACTION_CLICK:
            success = QtNativeAccessibility.clickAction(virtualViewId);
            if (success)
                sendEventForVirtualViewId(virtualViewId, AccessibilityEvent.TYPE_VIEW_CLICKED);
            break;
        case AccessibilityNodeInfo.ACTION_SCROLL_FORWARD:
            success = QtNativeAccessibility.scrollForward(virtualViewId);
            if (success)
                sendEventForVirtualViewId(virtualViewId, AccessibilityEvent.TYPE_VIEW_SCROLLED);
            break;
        case AccessibilityNodeInfo.ACTION_SCROLL_BACKWARD:
            success = QtNativeAccessibility.scrollBackward(virtualViewId);
            if (success)
                sendEventForVirtualViewId(virtualViewId, AccessibilityEvent.TYPE_VIEW_SCROLLED);
            break;
        }
        return success;
    }
}
