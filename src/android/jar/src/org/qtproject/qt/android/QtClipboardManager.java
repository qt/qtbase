// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

package org.qtproject.qt.android;

import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.util.Log;

import java.util.ArrayList;
import java.util.Objects;
import java.util.concurrent.Semaphore;

class QtClipboardManager
{
    static native void onClipboardDataChanged(long nativePointer);

    private final static String TAG = "QtClipboardManager";
    private ClipboardManager m_clipboardManager = null;
    private boolean m_usePrimaryClip = false;
    private final long m_nativePointer;

    QtClipboardManager(Context context, long nativePointer)
    {
        m_nativePointer = nativePointer;
        registerClipboardManager(context);
    }

    private void registerClipboardManager(Context context)
    {
        if (context != null) {
            final Semaphore semaphore = new Semaphore(0);
            QtNative.runAction(() -> {
                m_clipboardManager =
                        (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
                if (m_clipboardManager != null) {
                    m_clipboardManager.addPrimaryClipChangedListener(
                            () -> onClipboardDataChanged(m_nativePointer));
                }
                semaphore.release();
            });
            try {
                semaphore.acquire();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    @UsedFromNativeCode
    void clearClipData()
    {
        if (m_clipboardManager != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                m_clipboardManager.clearPrimaryClip();
            } else {
                String[] mimeTypes = { "application/octet-stream" };
                ClipData data = new ClipData("", mimeTypes, new ClipData.Item(new Intent()));
                m_clipboardManager.setPrimaryClip(data);
            }
        }
        m_usePrimaryClip = false;
    }

    @UsedFromNativeCode
    void setClipboardText(Context context, String text)
    {
        if (m_clipboardManager != null) {
            ClipData clipData = ClipData.newPlainText("text/plain", text);
            updatePrimaryClip(clipData, context);
        }
    }

    static boolean hasClipboardText(Context context)
    {
        ClipboardManager clipboardManager =
                (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);

        if (clipboardManager == null)
            return false;

        ClipDescription description = clipboardManager.getPrimaryClipDescription();
        // getPrimaryClipDescription can fail if the app does not have input focus
        if (description == null)
            return false;

        for (int i = 0; i < description.getMimeTypeCount(); ++i) {
            String itemMimeType = description.getMimeType(i);
            if (itemMimeType.matches("text/(.*)"))
                return true;
        }
        return false;
    }

    @UsedFromNativeCode
    boolean hasClipboardText()
    {
        return hasClipboardMimeType("text/(.*)");
    }

    @UsedFromNativeCode
    String getClipboardText()
    {
        try {
            if (m_clipboardManager != null && m_clipboardManager.hasPrimaryClip()) {
                ClipData primaryClip = m_clipboardManager.getPrimaryClip();
                if (primaryClip != null) {
                    for (int i = 0; i < primaryClip.getItemCount(); ++i)
                        if (primaryClip.getItemAt(i).getText() != null)
                            return primaryClip.getItemAt(i).getText().toString();
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to get clipboard data", e);
        }
        return "";
    }

    private void updatePrimaryClip(ClipData clipData, Context context)
    {
        try {
            if (m_usePrimaryClip) {
                ClipData clip = m_clipboardManager.getPrimaryClip();
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    Objects.requireNonNull(clip).addItem(context.getContentResolver(),
                            clipData.getItemAt(0));
                } else {
                    Objects.requireNonNull(clip).addItem(clipData.getItemAt(0));
                }
                m_clipboardManager.setPrimaryClip(clip);
            } else {
                m_clipboardManager.setPrimaryClip(clipData);
                m_usePrimaryClip = true;
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to set clipboard data", e);
        }
    }

    @UsedFromNativeCode
    void setClipboardHtml(Context context, String text, String html)
    {
        if (m_clipboardManager != null) {
            ClipData clipData = ClipData.newHtmlText("text/html", text, html);
            updatePrimaryClip(clipData, context);
        }
    }

    private boolean hasClipboardMimeType(String mimeType)
    {
        if (m_clipboardManager == null)
            return false;

        ClipDescription description = m_clipboardManager.getPrimaryClipDescription();
        // getPrimaryClipDescription can fail if the app does not have input focus
        if (description == null)
            return false;

        for (int i = 0; i < description.getMimeTypeCount(); ++i) {
            String itemMimeType = description.getMimeType(i);
            if (itemMimeType.matches(mimeType))
                return true;
        }
        return false;
    }

    @UsedFromNativeCode
    boolean hasClipboardHtml()
    {
        return hasClipboardMimeType("text/html");
    }

    @UsedFromNativeCode
    String getClipboardHtml()
    {
        try {
            if (m_clipboardManager != null && m_clipboardManager.hasPrimaryClip()) {
                ClipData primaryClip = m_clipboardManager.getPrimaryClip();
                if (primaryClip != null) {
                    for (int i = 0; i < primaryClip.getItemCount(); ++i)
                        if (primaryClip.getItemAt(i).getHtmlText() != null)
                            return primaryClip.getItemAt(i).getHtmlText();
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to get clipboard data", e);
        }
        return "";
    }

    @UsedFromNativeCode
    void setClipboardUri(Context context, String uriString)
    {
        if (m_clipboardManager != null) {
            ClipData clipData = ClipData.newUri(context.getContentResolver(), "text/uri-list",
                    Uri.parse(uriString));
            updatePrimaryClip(clipData, context);
        }
    }

    @UsedFromNativeCode
    boolean hasClipboardUri()
    {
        return hasClipboardMimeType("text/uri-list");
    }

    @UsedFromNativeCode
    private String[] getClipboardUris()
    {
        ArrayList<String> uris = new ArrayList<>();
        try {
            if (m_clipboardManager != null && m_clipboardManager.hasPrimaryClip()) {
                ClipData primaryClip = m_clipboardManager.getPrimaryClip();
                if (primaryClip != null) {
                    for (int i = 0; i < primaryClip.getItemCount(); ++i)
                        if (primaryClip.getItemAt(i).getUri() != null)
                            uris.add(primaryClip.getItemAt(i).getUri().toString());
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to get clipboard data", e);
        }
        String[] strings = new String[uris.size()];
        strings = uris.toArray(strings);
        return strings;
    }
}
