// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;

import android.database.Cursor;

import android.net.Uri;

import android.util.Log;

import android.os.ParcelFileDescriptor;
import android.os.Process;

import java.util.Arrays;
import java.util.stream.Collectors;

class QtContentFileEngine
{
    private static String QtTag = "QtContentFileEngine";

    static Cursor query(ContentResolver contentResolver, Uri uri, String[] projection,
                 String selection, String[] selectionArgs, String sortOrder)
    {
        try {
            // Check for read permission before doing any query
            if (!hasPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION))
                return null;

            return contentResolver.query(uri, projection, selection, selectionArgs, sortOrder);
        } catch (Exception ignored) {
            // Uncomment for debug only
            // String projections = Arrays.stream(projection).collect(Collectors.joining(", "));
            // Log.w(QtTag, "Query for [" + projections + "] failed with " + ignored);
        }

        return null;
    }

    static ParcelFileDescriptor openFileDescriptor(ContentResolver contentResolver,
                                                   Uri uri, String openMode)
    {
        try {
            int permissions = 0;
            if (openMode.contains("w"))
                permissions |= Intent.FLAG_GRANT_WRITE_URI_PERMISSION;
            if (openMode.contains("r"))
                permissions |= Intent.FLAG_GRANT_READ_URI_PERMISSION;
            // Check for relevant permission before attempting to obtain a file description
            if (!hasPermission(uri, permissions)) {
                Log.w(QtTag, "openFileDescriptor(): No permission for URI " + uri);
                return null;
            }

            return contentResolver.openFileDescriptor(uri, openMode);
        } catch (Exception e) {
            Log.w(QtTag, "openFileDescriptor() failed with " + e);
        }

        return null;
    }

    static boolean hasPermission(Uri uri, int permission)
    {
        Context context = QtNative.getContext();
        int status = context.checkUriPermission(uri, Process.myPid(), Process.myUid(), permission);

        return status == PackageManager.PERMISSION_GRANTED;
    }
}
