// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

package org.qtproject.qt.android;

import android.app.Activity;
import android.content.ClipData;
import android.content.ClipDescription;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Point;
import android.net.Uri;
import android.util.Log;
import android.view.DragAndDropPermissions;
import android.view.DragEvent;
import android.view.View;

import java.util.Arrays;

class QtDragManager implements View.OnDragListener
{
    private static final String TAG = "QtDragManager";
    private static final String DEFAULT_MIME_TYPE = "application/octet-stream";

    private static QtDragManager m_instance = null;

    private volatile long m_nativePointer = 0; // QAndroidPlatformDrag pointer
    private volatile View m_sourceView = null;
    private DragAndDropPermissions m_dragPermissions = null;

    static native boolean onDragEvent(long nativePointer, int viewId, int action, float x, float y,
                                      String[] mimeTypes, String[] clipData, boolean result);

    @UsedFromNativeCode
    static synchronized QtDragManager getInstance()
    {
        if (m_instance == null)
            m_instance = new QtDragManager();
        return m_instance;
    }

    @UsedFromNativeCode
    static synchronized void setNativePointer(long nativePointer)
    {
        getInstance().m_nativePointer = nativePointer;
    }

    @UsedFromNativeCode
    static synchronized void clearNativePointer(long expected)
    {
        final QtDragManager instance = getInstance();
        if (instance.m_nativePointer == expected) {
            instance.m_nativePointer = 0;
            // Drop the source reference so a torn-down drag is not kept alive by
            // the process-lifetime singleton. Permissions are released on drop or
            // drag-ended and auto-revoked when the activity is destroyed.
            instance.m_sourceView = null;
        }
    }

    @UsedFromNativeCode
    void startDrag(QtWindow sourceWindow, String[] mimeTypes, String[] clipData,
                   Bitmap shadowBitmap, int hotSpotX, int hotSpotY)
    {
        if (sourceWindow == null)
            return;

        // Post even when inactive so the drag loop is never left waiting on a parked action.
        QtNative.runAction(() -> {
            boolean started = false;
            try {
                String text = null;
                String html = null;
                Uri uri = null;
                String uriListData = null;
                boolean hasContentUri = false;
                for (int i = 0; i < mimeTypes.length; ++i) {
                    final String data = clipData[i];
                    final String mime = mimeTypes[i];
                    if (mime.equals(ClipDescription.MIMETYPE_TEXT_HTML)) {
                        html = data;
                    } else if (mime.equals(ClipDescription.MIMETYPE_TEXT_PLAIN)) {
                        text = data;
                    } else if (mime.equals(ClipDescription.MIMETYPE_TEXT_URILIST)
                               && !data.isEmpty()) {
                        final String first = data.split("[\\r\\n]+", 2)[0].trim();
                        if (!first.isEmpty()) {
                            uriListData = data;
                            uri = Uri.parse(first);
                            hasContentUri = "content".equals(uri.getScheme());
                        }
                    }
                }

                boolean hasMimeTypes = mimeTypes.length > 0;
                String[] types = hasMimeTypes ? mimeTypes : new String[]{ DEFAULT_MIME_TYPE };
                ClipData.Item item = new ClipData.Item(text, html, null, uri);
                ClipData clip = new ClipData(new ClipDescription("DragClip", types), item);
                // Add one item per extra URI so receivers that iterate items see all files.
                if (uriListData != null) {
                    final String[] parts = uriListData.split("[\\r\\n]+");
                    for (int i = 1; i < parts.length; ++i) {
                        final String part = parts[i].trim();
                        if (!part.isEmpty()) {
                            final Uri extraUri = Uri.parse(part);
                            hasContentUri |= "content".equals(extraUri.getScheme());
                            clip.addItem(new ClipData.Item(extraUri));
                        }
                    }
                }
                View.DragShadowBuilder shadow = new QtDragShadowBuilder(sourceWindow, shadowBitmap,
                                                                        hotSpotX, hotSpotY);
                m_sourceView = sourceWindow;

                // Cross into other apps, granting read access only for content URIs.
                int flags = View.DRAG_FLAG_GLOBAL;
                if (hasContentUri)
                    flags |= View.DRAG_FLAG_GLOBAL_URI_READ;

                started = sourceWindow.startDragAndDrop(clip, shadow, QtDragManager.this, flags);
            } catch (Exception e) {
                Log.e(TAG, "startDragAndDrop() failed on window id " + sourceWindow.getId(), e);
            }

            // Wake the native drag loop if the drag never started, otherwise drag() hangs.
            if (!started) {
                m_sourceView = null;
                final long ptr = m_nativePointer;
                if (ptr != 0) {
                    final String[] empty = new String[0];
                    onDragEvent(ptr, sourceWindow.getId(), DragEvent.ACTION_DRAG_ENDED,
                                0f, 0f, empty, empty, false);
                }
            }
        }, false);
    }

    @UsedFromNativeCode
    void cancelDrag()
    {
        QtNative.runAction(() -> {
            if (m_sourceView != null)
                m_sourceView.cancelDragAndDrop();
        });
    }

    private void releaseDragPermissions()
    {
        if (m_dragPermissions != null) {
            m_dragPermissions.release();
            m_dragPermissions = null;
        }
    }

    void onSourceWindowDetached(View view)
    {
        if (m_sourceView != view)
            return;
        m_sourceView = null;
        final long ptr = m_nativePointer;
        if (ptr != 0) {
            final String[] empty = new String[0];
            onDragEvent(ptr, view.getId(), DragEvent.ACTION_DRAG_ENDED, 0f, 0f, empty, empty, false);
        }
    }

    @Override
    public boolean onDrag(View view, DragEvent event)
    {
        final int action = event.getAction();

        if (action == DragEvent.ACTION_DRAG_ENDED) {
            m_sourceView = null;
            releaseDragPermissions();
        }

        final long nativePointer = m_nativePointer;
        if (nativePointer == 0)
            return false;

        if (action == DragEvent.ACTION_DRAG_STARTED)
            return true;

        String[] mimeTypes = new String[0];
        String[] clipData = new String[0];
        final ClipDescription description = event.getClipDescription();
        if (description != null) {
            final int size = description.getMimeTypeCount();
            mimeTypes = new String[size];
            for (int i = 0; i < size; ++i)
                mimeTypes[i] = description.getMimeType(i);
        }

        // Clip data is only readable on ACTION_DROP and move events carry types only.
        if (action == DragEvent.ACTION_DROP) {
            try {
                final ClipData clip = event.getClipData();
                if (clip != null && clip.getItemCount() > 0) {
                    clipData = new String[mimeTypes.length];
                    Arrays.fill(clipData, "");
                    // URIs span all items (one per file)
                    StringBuilder uriList = new StringBuilder();
                    for (int i = 0; i < clip.getItemCount(); ++i) {
                        Uri itemUri = clip.getItemAt(i).getUri();
                        if (itemUri != null) {
                            if (uriList.length() > 0)
                                uriList.append('\n');
                            uriList.append(itemUri.toString());
                        }
                    }
                    if (uriList.length() > 0) {
                        releaseDragPermissions();
                        final Activity activity = QtNative.activity();
                        if (activity != null) {
                            m_dragPermissions = activity.requestDragAndDropPermissions(event);
                            if (m_dragPermissions == null)
                                Log.w(TAG, "Drag and drop reading permissions denied.");
                        }
                    }

                    // text/HTML are in item 0 only
                    ClipData.Item item = clip.getItemAt(0);
                    for (int i = 0; i < mimeTypes.length; ++i) {
                        if (ClipDescription.MIMETYPE_TEXT_HTML.equals(mimeTypes[i])) {
                            final String html = item.getHtmlText();
                            clipData[i] = (html != null) ? html : "";
                        } else if (ClipDescription.MIMETYPE_TEXT_URILIST.equals(mimeTypes[i])) {
                            clipData[i] = uriList.toString();
                        } else if (ClipDescription.MIMETYPE_TEXT_PLAIN.equals(mimeTypes[i])) {
                            // An item can carry both a URI and a text label, so prefer the label.
                            CharSequence text = item.getText();
                            if (text == null)
                                text = item.coerceToText(view.getContext());
                            clipData[i] = (text != null) ? text.toString() : "";
                        } else {
                            final Uri itemUri = item.getUri();
                            if (itemUri != null) {
                                clipData[i] = itemUri.toString();
                            } else {
                                final CharSequence text = item.coerceToText(view.getContext());
                                clipData[i] = (text != null) ? text.toString() : "";
                            }
                        }
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to read dropped clip data", e);
            }
        }

        return onDragEvent(nativePointer, view.getId(), action, event.getX(), event.getY(),
                           mimeTypes, clipData, event.getResult());
    }

    private static class QtDragShadowBuilder extends View.DragShadowBuilder
    {
        private final Bitmap m_bitmap;
        private final int m_hotSpotX;
        private final int m_hotSpotY;

        QtDragShadowBuilder(View view, Bitmap bitmap, int hotSpotX, int hotSpotY)
        {
            super(view);
            m_bitmap = bitmap;
            m_hotSpotX = hotSpotX;
            m_hotSpotY = hotSpotY;
        }

        @Override
        public void onProvideShadowMetrics(Point outShadowSize, Point outShadowTouchPoint)
        {
            if (m_bitmap == null) {
                super.onProvideShadowMetrics(outShadowSize, outShadowTouchPoint);
                return;
            }

            outShadowSize.set(m_bitmap.getWidth(), m_bitmap.getHeight());
            outShadowTouchPoint.set(m_hotSpotX, m_hotSpotY);
        }

        @Override
        public void onDrawShadow(Canvas canvas)
        {
            if (m_bitmap != null)
                canvas.drawBitmap(m_bitmap, 0, 0, null);
        }
    }
}
